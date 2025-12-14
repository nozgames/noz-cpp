//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "pch.h"
#include "platform.h"

constexpr int MAX_CONCURRENT_REQUESTS = 16;

enum class HttpRequestState {
    Idle,
    Queued,
    Active,
};

struct HttpRequestImpl : HttpRequest {
    PlatformHttpHandle handle;
    HttpCallback callback;
    HttpStatus last_status;
    char *response_string;
    HttpRequestState state;
    String1024 url;

    // Stored request data for deferred execution
    char *body;
    u32 body_size;
    char *content_type;
    char *headers;
    char *method;  // "GET", "POST", "PUT"

    HttpRequestImpl *next;  // For queue linked list
};

static HttpRequestImpl *g_active_head = nullptr;
static HttpRequestImpl *g_queue_head = nullptr;
static HttpRequestImpl *g_queue_tail = nullptr;
static int g_active_count = 0;

static HttpRequestImpl *AllocRequest() {
    HttpRequestImpl *req = (HttpRequestImpl *)Alloc(ALLOCATOR_DEFAULT, sizeof(HttpRequestImpl));
    *req = {};
    return req;
}

static void QueuePush(HttpRequestImpl *req) {
    req->next = nullptr;
    if (g_queue_tail) {
        g_queue_tail->next = req;
        g_queue_tail = req;
    } else {
        g_queue_head = g_queue_tail = req;
    }
    req->state = HttpRequestState::Queued;
}

static HttpRequestImpl *QueuePop() {
    if (!g_queue_head)
        return nullptr;

    HttpRequestImpl *req = g_queue_head;
    g_queue_head = req->next;
    if (!g_queue_head)
        g_queue_tail = nullptr;
    req->next = nullptr;
    return req;
}

static void ActiveListAdd(HttpRequestImpl *req) {
    req->next = g_active_head;
    g_active_head = req;
    g_active_count++;
    req->state = HttpRequestState::Active;
}

static void ActiveListRemove(HttpRequestImpl *req) {
    HttpRequestImpl **pp = &g_active_head;
    while (*pp) {
        if (*pp == req) {
            *pp = req->next;
            g_active_count--;
            req->next = nullptr;
            return;
        }
        pp = &(*pp)->next;
    }
}

static void StartRequest(HttpRequestImpl *req) {
    if (req->method && strcmp(req->method, "GET") == 0) {
        req->handle = PlatformGetURL(req->url);
    } else {
        req->handle = PlatformPostURL(req->url, req->body, req->body_size,
                                       req->content_type, req->headers, req->method);
    }
    req->last_status = HttpStatus::Pending;
    ActiveListAdd(req);

    // Free stored body data after sending
    if (req->body) {
        Free(req->body);
        req->body = nullptr;
    }
    if (req->content_type) {
        Free(req->content_type);
        req->content_type = nullptr;
    }
    if (req->headers) {
        Free(req->headers);
        req->headers = nullptr;
    }
    if (req->method) {
        Free(req->method);
        req->method = nullptr;
    }
}

static void TryStartQueued() {
    while (g_active_count < MAX_CONCURRENT_REQUESTS) {
        HttpRequestImpl *req = QueuePop();
        if (!req)
            break;
        StartRequest(req);
    }
}

const char* GetUrl(HttpRequest* request) {
    if (!request) return nullptr;
    return static_cast<HttpRequestImpl*>(request)->url;
}

static char *DupString(const char *str) {
    if (!str) return nullptr;
    size_t len = strlen(str) + 1;
    char *copy = static_cast<char *>(Alloc(ALLOCATOR_DEFAULT, (int)len));
    memcpy(copy, str, len);
    return copy;
}

HttpRequest *GetUrl(const char *url, HttpCallback on_complete) {
    HttpRequestImpl *req = AllocRequest();
    Set(req->url, url);
    req->callback = on_complete;
    req->method = DupString("GET");

    if (g_active_count < MAX_CONCURRENT_REQUESTS) {
        StartRequest(req);
    } else {
        QueuePush(req);
    }

    return req;
}

static void SetupPostRequest(HttpRequestImpl *req, const char *url, const void *body, u32 body_size,
                              const char *content_type, const char *headers, const char *method,
                              HttpCallback on_complete) {
    Set(req->url, url);
    req->callback = on_complete;
    req->method = DupString(method);
    req->content_type = DupString(content_type);
    req->headers = DupString(headers);

    if (body && body_size > 0) {
        req->body = (char *)Alloc(ALLOCATOR_DEFAULT, body_size);
        memcpy(req->body, body, body_size);
        req->body_size = body_size;
    }

    if (g_active_count < MAX_CONCURRENT_REQUESTS) {
        StartRequest(req);
    } else {
        QueuePush(req);
    }
}

HttpRequest *PostUrl(const char *url, const void *body, u32 body_size, const char *content_type,
                     const char *headers, HttpCallback on_complete) {
    HttpRequestImpl *req = AllocRequest();
    SetupPostRequest(req, url, body, body_size, content_type, headers, "POST", on_complete);
    return req;
}

HttpRequest *PutUrl(const char *url, const void *body, u32 body_size, const char *content_type,
                    const char *headers, HttpCallback on_complete) {
    HttpRequestImpl *req = AllocRequest();
    SetupPostRequest(req, url, body, body_size, content_type, headers, "PUT", on_complete);
    return req;
}


HttpRequest *HttpPostString(const char *url, const char *body, const char *content_type, HttpCallback on_complete) {
    return PostUrl(url, body, (u32) strlen(body), content_type, nullptr, on_complete);
}

HttpRequest *HttpPostJson(const char *url, const char *json, HttpCallback on_complete) {
    return PostUrl(url, json, (u32) strlen(json), "application/json", nullptr, on_complete);
}

HttpStatus HttpGetStatus(HttpRequest *request) {
    if (!request)
        return HttpStatus::None;

    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);

    // Queued requests are pending but not yet started
    if (req->state == HttpRequestState::Queued)
        return HttpStatus::Pending;

    if (req->state == HttpRequestState::Idle)
        return HttpStatus::None;

    HttpStatus pstatus = PlatformGetStatus(req->handle);

    switch (pstatus) {
        case HttpStatus::None: return HttpStatus::None;
        case HttpStatus::Pending: return HttpStatus::Pending;
        case HttpStatus::Complete: return HttpStatus::Complete;
        case HttpStatus::Error: return HttpStatus::Error;
    }

    return HttpStatus::None;
}

int GetResponseStatusCode(HttpRequest *request) {
    if (!request)
        return 0;

    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);
    return PlatformGetStatusCode(req->handle);
}

bool HttpIsComplete(HttpRequest *request) {
    HttpStatus status = HttpGetStatus(request);
    return status == HttpStatus::Complete || status == HttpStatus::Error;
}

bool HttpIsSuccess(HttpRequest *request) {
    if (HttpGetStatus(request) != HttpStatus::Complete)
        return false;

    int code = GetResponseStatusCode(request);
    return code >= 200 && code < 300;
}

Stream *GetResponseStream(HttpRequest *request, Allocator *allocator) {
    if (!request) return nullptr;
    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);
    u32 out_size = 0;
    const u8 *res = PlatformGetResponse(req->handle, &out_size);
    return LoadStream(allocator, res, out_size);
}

const char *HttpGetResponseString(HttpRequest *request) {
    if (!request)
        return nullptr;

    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);

    // Return cached string if available
    if (req->response_string)
        return req->response_string;

    u32 size = 0;
    const u8 *data = PlatformGetResponse(req->handle, &size);
    if (!data || size == 0)
        return nullptr;

    // Allocate and null-terminate
    req->response_string = static_cast<char *>(Alloc(ALLOCATOR_DEFAULT, size + 1));
    memcpy(req->response_string, data, size);
    req->response_string[size] = '\0';

    return req->response_string;
}

u32 HttpGetResponseSize(HttpRequest *request) {
    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);
    u32 size = 0;
    PlatformGetResponse(req->handle, &size);
    return size;
}

char* GetResponseHeader(HttpRequest *request, const char *name, Allocator *allocator) {
    if (!request)
        return nullptr;

    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);
    return PlatformGetResponseHeader(req->handle, name, allocator);
}

static void QueueRemove(HttpRequestImpl *req) {
    HttpRequestImpl **pp = &g_queue_head;
    while (*pp) {
        if (*pp == req) {
            *pp = req->next;
            if (req == g_queue_tail)
                g_queue_tail = nullptr;
            req->next = nullptr;
            return;
        }
        pp = &(*pp)->next;
    }
}

void HttpCancel(HttpRequest *request) {
    if (!request)
        return;

    HttpRequestImpl *req = (HttpRequestImpl *)request;

    if (req->state == HttpRequestState::Active) {
        PlatformCancel(req->handle);
        ActiveListRemove(req);
    } else if (req->state == HttpRequestState::Queued) {
        QueueRemove(req);
    }

    req->state = HttpRequestState::Idle;
    req->last_status = HttpStatus::None;
}

static void FreeRequestData(HttpRequestImpl *req) {
    PlatformFree(req->handle);

    if (req->response_string) {
        Free(req->response_string);
        req->response_string = nullptr;
    }
    if (req->body) {
        Free(req->body);
        req->body = nullptr;
    }
    if (req->content_type) {
        Free(req->content_type);
        req->content_type = nullptr;
    }
    if (req->headers) {
        Free(req->headers);
        req->headers = nullptr;
    }
    if (req->method) {
        Free(req->method);
        req->method = nullptr;
    }
}

void Free(HttpRequest *request) {
    if (!request)
        return;

    HttpRequestImpl *req = (HttpRequestImpl *)request;

    if (req->state == HttpRequestState::Active) {
        ActiveListRemove(req);
    } else if (req->state == HttpRequestState::Queued) {
        QueueRemove(req);
    }

    FreeRequestData(req);

    TryStartQueued();
}

void InitHttp() {
    g_active_head = nullptr;
    g_queue_head = nullptr;
    g_queue_tail = nullptr;
    g_active_count = 0;

    PlatformInitHttp();
}

void ShutdownHttp() {
    // Free all active requests
    while (g_active_head) {
        HttpRequestImpl *req = g_active_head;
        g_active_head = req->next;
        FreeRequestData(req);
        Free(req);
    }

    // Free all queued requests
    while (g_queue_head) {
        HttpRequestImpl *req = g_queue_head;
        g_queue_head = req->next;
        FreeRequestData(req);
        Free(req);
    }

    g_queue_tail = nullptr;
    g_active_count = 0;

    PlatformShutdownHttp();
}

void UpdateHttp() {
    PlatformUpdateHttp();

    HttpRequestImpl *req = g_active_head;
    while (req) {
        HttpRequestImpl *next = req->next;

        HttpStatus current = HttpGetStatus((HttpRequest *)req);

        // Check for state transition to complete/error/none
        if (req->last_status == HttpStatus::Pending && current != HttpStatus::Pending) {
            // Treat None as Error (platform may return None on failure)
            req->last_status = (current == HttpStatus::Complete) ? HttpStatus::Complete : HttpStatus::Error;

            // Remove from active list to free up slot for queued requests
            ActiveListRemove(req);
            req->state = HttpRequestState::Idle;

            // Fire callback
            if (req->callback)
                req->callback((HttpRequest *)req);
        }

        req = next;
    }

    TryStartQueued();
}

void EncodeUrl(Text& out, const Text& input) {
    PlatformEncodeUrl(out.value, TEXT_MAX_LENGTH + 1, input.value, input.length);
    out.length = (int)strlen(out.value);
}

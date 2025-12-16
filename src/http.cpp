//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "pch.h"
#include "platform.h"
#include <noz/task.h>

constexpr int MAX_CONCURRENT_REQUESTS = 16;

enum class HttpRequestState {
    Idle,
    Queued,
    Active,
};

struct HttpRequestImpl : HttpRequest {
    PlatformHttpHandle handle;
    noz::TaskHandle task;
    HttpCallback callback;
    char *response_string;
    HttpRequestState state;
    HttpStatus last_status;
    int cached_status_code;  // Cached on completion for thread-safe access
    u8 *cached_response;     // Cached response data for thread-safe access
    u32 cached_response_size;
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
    req->task = noz::TASK_HANDLE_INVALID;
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

static char *DupString(const char *str) {
    if (!str) return nullptr;
    size_t len = strlen(str) + 1;
    char *copy = static_cast<char *>(Alloc(ALLOCATOR_DEFAULT, (int)len));
    memcpy(copy, str, len);
    return copy;
}

noz::TaskHandle GetUrl(const char *url, HttpCallback on_complete) {
    HttpRequestImpl *req = AllocRequest();
    Set(req->url, url);
    req->method = DupString("GET");
    req->callback = on_complete;
    req->task = noz::CreateVirtualTask();

    if (g_active_count < MAX_CONCURRENT_REQUESTS) {
        StartRequest(req);
    } else {
        QueuePush(req);
    }

    return req->task;
}

static noz::TaskHandle SetupPostRequest(HttpRequestImpl *req, const char *url, const void *body, u32 body_size,
                              const char *content_type, const char *headers, const char *method,
                              HttpCallback on_complete) {
    Set(req->url, url);
    req->method = DupString(method);
    req->content_type = DupString(content_type);
    req->headers = DupString(headers);
    req->callback = on_complete;
    req->task = noz::CreateVirtualTask();

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

    return req->task;
}

noz::TaskHandle PostUrl(
    const char *url,
    const void *body,
    u32 body_size,
    const char *content_type,
    const char *headers,
    const HttpCallback &on_complete) {
    HttpRequestImpl *req = AllocRequest();
    return SetupPostRequest(req, url, body, body_size, content_type, headers, "POST", on_complete);
}

noz::TaskHandle PutUrl(const char *url, const void *body, u32 body_size, const char *content_type,
                       const char *headers, const HttpCallback &on_complete) {
    HttpRequestImpl *req = AllocRequest();
    return SetupPostRequest(req, url, body, body_size, content_type, headers, "PUT", on_complete);
}

const char* GetRequestUrl(HttpRequest* request) {
    if (!request) return nullptr;
    return static_cast<HttpRequestImpl*>(request)->url;
}

int GetStatusCode(HttpRequest *request) {
    if (!request)
        return 0;

    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);
    return req->cached_status_code;
}

bool IsSuccess(HttpRequest *request) {
    int code = GetStatusCode(request);
    return code >= 200 && code < 300;
}

Stream *GetResponseStream(HttpRequest *request, Allocator *allocator) {
    if (!request) return nullptr;
    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);
    return LoadStream(allocator, req->cached_response, req->cached_response_size);
}

const char *GetResponseString(HttpRequest *request) {
    if (!request)
        return nullptr;

    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);

    // Return cached string if available
    if (req->response_string)
        return req->response_string;

    if (!req->cached_response || req->cached_response_size == 0)
        return nullptr;

    // Allocate and null-terminate
    req->response_string = static_cast<char *>(Alloc(ALLOCATOR_DEFAULT, req->cached_response_size + 1));
    memcpy(req->response_string, req->cached_response, req->cached_response_size);
    req->response_string[req->cached_response_size] = '\0';

    return req->response_string;
}

u32 GetResponseSize(HttpRequest *request) {
    if (!request) return 0;
    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);
    return req->cached_response_size;
}

char* GetResponseHeader(HttpRequest *request, const char *name, Allocator *allocator) {
    if (!request)
        return nullptr;

    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);
    return PlatformGetResponseHeader(req->handle, name, allocator);
}

static void FreeRequestData(HttpRequestImpl *req) {
    PlatformFree(req->handle);

    if (req->response_string) {
        Free(req->response_string);
        req->response_string = nullptr;
    }
    if (req->cached_response) {
        Free(req->cached_response);
        req->cached_response = nullptr;
        req->cached_response_size = 0;
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

        HttpStatus current = PlatformGetStatus(req->handle);

        // Only process state transition from Pending to Complete/Error
        if (req->last_status == HttpStatus::Pending && current != HttpStatus::Pending) {
            // Update last_status - treat None as Error
            req->last_status = (current == HttpStatus::Complete) ? HttpStatus::Complete : HttpStatus::Error;

            // Cache status code for thread-safe access
            req->cached_status_code = PlatformGetStatusCode(req->handle);

            // Cache response data for thread-safe access
            u32 response_size = 0;
            const u8 *response_data = PlatformGetResponse(req->handle, &response_size);
            if (response_data && response_size > 0) {
                req->cached_response = static_cast<u8*>(Alloc(ALLOCATOR_DEFAULT, response_size));
                memcpy(req->cached_response, response_data, response_size);
                req->cached_response_size = response_size;
            } else {
                req->cached_response = nullptr;
                req->cached_response_size = 0;
            }

            // Remove from active list
            ActiveListRemove(req);
            req->state = HttpRequestState::Idle;

            // Call callback if provided
            if (req->callback) {
                req->callback(req);
            }

            // Complete the virtual task with HttpRequest* as result
            if (req->task) {
                noz::CompleteTask(req->task, req);
            }
        }

        req = next;
    }

    TryStartQueued();
}

void EncodeUrl(Text& out, const Text& input) {
    PlatformEncodeUrl(out.value, TEXT_MAX_LENGTH + 1, input.value, input.length);
    out.length = (int)strlen(out.value);
}

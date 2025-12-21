//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "pch.h"
#include "platform.h"
#include <noz/task.h>

namespace noz {
    enum HttpRequestState : u8 {
        HTTP_REQUEST_STATE_NONE,
        HTTP_REQUEST_STATE_QUEUED,
        HTTP_REQUEST_STATE_ACTIVE,
        HTTP_REQUEST_STATE_COMPLETE
    };

    enum HttpRequestMethod : u8 {
        HTTP_REQUEST_METHOD_NONE,
        HTTP_REQUEST_METHOD_GET,
        HTTP_REQUEST_METHOD_PUT,
        HTTP_REQUEST_METHOD_POST,
    };

    struct QueuedHttpRequest {

    };

    struct HttpRequestImpl : HttpRequest {
        String1024 url;
        String64 content_type;
        String128 headers;
        Task task;
        HttpRequestMethod method;
        HttpRequestState state;
        HttpCallback callback;
        PlatformHttpHandle handle;
        int status_code;
        u8 *body;
        u32 body_size;
        Stream* response;
    };

    struct HttpSystem {
        int max_concurrent_requests;
        int max_requests;
        HttpRequestImpl* requests;
        int request_count;
    };

    extern void InitHttp(const ApplicationTraits& traits);
    extern void ShutdownHttp();
    extern void UpdateHttp();
}

static noz::HttpSystem g_http = {};

using namespace noz;

static void FreeRequestData(void* result) {
    HttpRequestImpl* req = static_cast<HttpRequestImpl*>(result);
    if (!req) return;

    Free(req->response);
    Free(req->body);
    req->response = nullptr;
    req->body = nullptr;
    req->state = HTTP_REQUEST_STATE_NONE;
    g_http.request_count--;
}

static HttpRequestImpl* GetRequest(const char* url, HttpRequestMethod method, Task parent, const HttpCallback& callback) {
    if (g_http.request_count >= g_http.max_requests)
        return nullptr;

    int request_index = 0;
    for (; request_index < g_http.max_requests && g_http.requests[request_index].state != HTTP_REQUEST_STATE_NONE; request_index++) {}
    assert(request_index < g_http.max_requests);

    HttpRequestImpl* request = g_http.requests + request_index;
    Set(request->url, url);
    request->task = CreateTask({.destroy = FreeRequestData, .parent = parent});
    request->method = method;
    request->callback = callback;
    request->state = HTTP_REQUEST_STATE_QUEUED;
    request->body = nullptr;
    request->response = nullptr;

    g_http.request_count++;

    return request;
}

static void StartRequest(HttpRequestImpl* request) {
    if (request->method == HTTP_REQUEST_METHOD_GET) {
        request->handle = PlatformGetURL(request->url);
    } else if (request->method == HTTP_REQUEST_METHOD_PUT) {
        request->handle = PlatformPostURL(request->url, request->body, request->body_size, request->content_type, request->headers, "PUT");
    } else if (request->method == HTTP_REQUEST_METHOD_POST) {
        request->handle = PlatformPostURL(request->url, request->body, request->body_size, request->content_type, request->headers, "POST");
    } else {
        assert(false && "unknown method type");
        return;
    }

    Free(request->body);

    request->state = HTTP_REQUEST_STATE_ACTIVE;
    request->body = nullptr;
}

Task noz::GetUrl(const char *url, Task parent, const HttpCallback& callback) {
    HttpRequestImpl* req = GetRequest(url, HTTP_REQUEST_METHOD_GET, parent, callback);
    if (!req) return nullptr;
    return req->task;
}

static Task PostUrlInternal(
    const char *url,
    HttpRequestMethod method,
    const u8* body,
    u32 body_size,
    const char *content_type,
    const char *headers,
    Task parent,
    const HttpCallback &callback) {
    HttpRequestImpl* req = GetRequest(url, method, parent, callback);
    if (!req) return nullptr;
    Set(req->content_type, content_type);
    Set(req->headers, headers);
    req->body = static_cast<u8*>(Alloc(ALLOCATOR_DEFAULT, body_size));
    memcpy(req->body, body, body_size);
    req->body_size = body_size;
    return req->task;
}

Task noz::PostUrl(
    const char *url,
    const void *body,
    u32 body_size,
    const char *content_type,
    const char *headers,
    Task parent,
    const HttpCallback &callback) {
    return PostUrlInternal(url, HTTP_REQUEST_METHOD_POST, static_cast<const u8*>(body), body_size, content_type, headers, parent, callback);
}

Task noz::PutUrl(
    const char *url,
    const void *body,
    u32 body_size,
    const char *content_type,
    const char *headers,
    Task parent,
    const HttpCallback &callback) {
    return PostUrlInternal(url, HTTP_REQUEST_METHOD_PUT, static_cast<const u8*>(body), body_size, content_type, headers, parent, callback);
}

const char* noz::GetUrl(HttpRequest* request) {
    if (!request) return nullptr;
    return static_cast<HttpRequestImpl*>(request)->url;
}

int noz::GetStatusCode(HttpRequest *request) {
    if (!request) return 0;
    return static_cast<HttpRequestImpl*>(request)->status_code;
}

bool noz::IsSuccess(HttpRequest *request) {
    int code = GetStatusCode(request);
    return code >= 200 && code < 300;
}

Stream* noz::GetResponseStream(HttpRequest *request) {
    if (!request) return nullptr;
    return static_cast<HttpRequestImpl*>(request)->response;
}

Stream* noz::ReleaseResponseStream(HttpRequest* request) {
    if (!request) return nullptr;
    HttpRequestImpl* req = static_cast<HttpRequestImpl*>(request);
    Stream* response = req->response;
    req->response = nullptr;
    return response;
}

char* noz::GetResponseHeader(HttpRequest *request, const char *name, Allocator *allocator) {
    if (!request)
        return nullptr;

    HttpRequestImpl *req = static_cast<HttpRequestImpl *>(request);
    return PlatformGetResponseHeader(req->handle, name, allocator);
}

void noz::EncodeUrl(Text& out, const Text& input) {
    PlatformEncodeUrl(out.value, TEXT_MAX_LENGTH + 1, input.value, input.length);
    out.length = Length(out.value);
}

static void FinishRequest(HttpRequestImpl* request) {
    assert(request);
    assert(PlatformGetStatus(request->handle) != PLATFORM_HTTP_STATUS_PENDING);

    request->state = HTTP_REQUEST_STATE_COMPLETE;
    request->status_code = PlatformGetStatusCode(request->handle);
    request->response = PlatformReleaseResponseStream(request->handle);
    PlatformFree(request->handle);
    request->handle = {};

    if (request->callback)
        request->callback(request->task, request);

    Complete(request->task, request);
}

void noz::UpdateHttp() {
    if (g_http.request_count == 0)
        return;

    PlatformUpdateHttp();

    int active_count = 0;

    for (int request_index=0, request_count=g_http.request_count; request_index < g_http.max_requests && request_count; request_index++) {
        HttpRequestImpl* request = g_http.requests + request_index;

        // Check if parent task was canceled - abort the request
        Task parent = GetParent(request->task);
        if (parent && IsCancelled(parent)) {
            if (request->state == HTTP_REQUEST_STATE_ACTIVE) {
                PlatformCancel(request->handle);
                PlatformFree(request->handle);
                request->handle = {};
            }
            Cancel(request->task);
            request->state = HTTP_REQUEST_STATE_NONE;
            g_http.request_count--;
            continue;
        }

        // @active
        if (request->state == HTTP_REQUEST_STATE_ACTIVE) {
            if (PlatformGetStatus(request->handle) != PLATFORM_HTTP_STATUS_PENDING) {
                FinishRequest(request);
            } else {
                active_count++;
            }

        // @complete
        } else if (request->state == HTTP_REQUEST_STATE_COMPLETE) {
            if (!IsValid(request->task)) {
                request->state = HTTP_REQUEST_STATE_NONE;
                g_http.request_count--;
            }
        }
    }

    // Start queued requests if under limit
    if (active_count < g_http.max_concurrent_requests) {
        for (int request_index=0, request_count=g_http.request_count;
            request_index < g_http.max_requests && request_count && active_count < g_http.max_concurrent_requests;
            request_index++) {
            HttpRequestImpl* request = g_http.requests + request_index;
            if (request->state != HTTP_REQUEST_STATE_QUEUED) continue;
            StartRequest(request);
            active_count++;
        }
    }
}

void noz::InitHttp(const ApplicationTraits& traits) {
    g_http = {};
    g_http.max_concurrent_requests = traits.http.max_concurrent_requests;
    g_http.max_requests = traits.http.max_requests;
    g_http.requests = new HttpRequestImpl[g_http.max_requests];

    for (int request_index = 0; request_index < g_http.max_requests; request_index++) {
        HttpRequestImpl* request = g_http.requests + request_index;
        Clear(request->url);
        Clear(request->content_type);
        Clear(request->headers);
        request->task = nullptr;
        request->method = HTTP_REQUEST_METHOD_NONE;
        request->state = HTTP_REQUEST_STATE_NONE;
        request->callback = nullptr;
        request->handle = {};
        request->status_code = 0;
        request->body = nullptr;
        request->body_size = 0;
        request->response = nullptr;
    }

    PlatformInitHttp(traits);
}

void noz::ShutdownHttp() {
    // while (g_http.active_head) {
    //     HttpRequestImpl *req = g_http.active_head;
    //     g_http.active_head = req->next;
    //     FreeRequestData(req);
    //     Free(req);
    // }
    //
    // while (g_http.queue_head) {
    //     HttpRequestImpl *req = g_http.queue_head;
    //     g_http.queue_head = req->next;
    //     FreeRequestData(req);
    //     Free(req);
    // }

    delete[] g_http.requests;
    g_http.requests = nullptr;
    g_http.max_concurrent_requests = 0;
    g_http.max_requests = 0;
    //g_http.active_count = 0;

    PlatformShutdownHttp();
}


//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "pch.h"
#include "platform.h"

constexpr int MAX_REQUESTS = 16;

struct HttpRequestImpl
{
    PlatformHttpHandle handle;
    HttpCallback callback;
    HttpStatus last_status;
    char* response_string;
    bool active;
};

static HttpRequestImpl g_requests[MAX_REQUESTS] = {};

static HttpRequestImpl* AllocRequest()
{
    for (int i = 0; i < MAX_REQUESTS; i++)
    {
        if (!g_requests[i].active)
        {
            g_requests[i] = {};
            g_requests[i].active = true;
            return &g_requests[i];
        }
    }
    LogWarning("No free HTTP request slots");
    return nullptr;
}

HttpRequest* HttpGet(const char* url, HttpCallback on_complete)
{
    HttpRequestImpl* req = AllocRequest();
    if (!req)
        return nullptr;

    req->handle = PlatformGetURL(url);
    req->callback = on_complete;
    req->last_status = HttpStatus::Pending;

    return (HttpRequest*)req;
}

HttpRequest* HttpPost(const char* url, const void* body, u32 body_size, const char* content_type, HttpCallback on_complete)
{
    HttpRequestImpl* req = AllocRequest();
    if (!req)
        return nullptr;

    req->handle = PlatformPostURL(url, body, body_size, content_type);
    req->callback = on_complete;
    req->last_status = HttpStatus::Pending;

    return (HttpRequest*)req;
}

HttpRequest* HttpPostString(const char* url, const char* body, const char* content_type, HttpCallback on_complete)
{
    return HttpPost(url, body, (u32)strlen(body), content_type, on_complete);
}

HttpRequest* HttpPostJson(const char* url, const char* json, HttpCallback on_complete)
{
    return HttpPost(url, json, (u32)strlen(json), "application/json", on_complete);
}

HttpStatus HttpGetStatus(HttpRequest* request)
{
    if (!request)
        return HttpStatus::None;

    HttpRequestImpl* req = (HttpRequestImpl*)request;
    HttpStatus pstatus = PlatformGetStatus(req->handle);

    switch (pstatus) {
        case HttpStatus::None:     return HttpStatus::None;
        case HttpStatus::Pending:  return HttpStatus::Pending;
        case HttpStatus::Complete: return HttpStatus::Complete;
        case HttpStatus::Error:    return HttpStatus::Error;
    }

    return HttpStatus::None;
}

int HttpGetStatusCode(HttpRequest* request)
{
    if (!request)
        return 0;

    HttpRequestImpl* req = (HttpRequestImpl*)request;
    return PlatformGetStatusCode(req->handle);
}

bool HttpIsComplete(HttpRequest* request)
{
    HttpStatus status = HttpGetStatus(request);
    return status == HttpStatus::Complete || status == HttpStatus::Error;
}

bool HttpIsSuccess(HttpRequest* request)
{
    if (HttpGetStatus(request) != HttpStatus::Complete)
        return false;

    int code = HttpGetStatusCode(request);
    return code >= 200 && code < 300;
}

const u8* HttpGetResponseData(HttpRequest* request, u32* out_size)
{
    if (!request)
    {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    HttpRequestImpl* req = (HttpRequestImpl*)request;
    return PlatformGetResponse(req->handle, out_size);
}

const char* HttpGetResponseString(HttpRequest* request)
{
    if (!request)
        return nullptr;

    HttpRequestImpl* req = (HttpRequestImpl*)request;

    // Return cached string if available
    if (req->response_string)
        return req->response_string;

    u32 size = 0;
    const u8* data = PlatformGetResponse(req->handle, &size);
    if (!data || size == 0)
        return nullptr;

    // Allocate and null-terminate
    req->response_string = (char*)Alloc(ALLOCATOR_DEFAULT, size + 1);
    memcpy(req->response_string, data, size);
    req->response_string[size] = '\0';

    return req->response_string;
}

u32 HttpGetResponseSize(HttpRequest* request)
{
    u32 size = 0;
    HttpGetResponseData(request, &size);
    return size;
}

void HttpCancel(HttpRequest* request)
{
    if (!request)
        return;

    HttpRequestImpl* req = (HttpRequestImpl*)request;
    PlatformCancel(req->handle);
    req->last_status = HttpStatus::None;
}

void HttpRelease(HttpRequest* request)
{
    if (!request)
        return;

    HttpRequestImpl* req = (HttpRequestImpl*)request;

    PlatformFree(req->handle);

    if (req->response_string)
    {
        Free(req->response_string);
        req->response_string = nullptr;
    }

    req->callback = nullptr;
    req->active = false;
}

void InitHttp()
{
    for (int i = 0; i < MAX_REQUESTS; i++)
        g_requests[i] = {};

    PlatformInitHttp();
}

void ShutdownHttp()
{
    for (int i = 0; i < MAX_REQUESTS; i++)
    {
        if (g_requests[i].active)
        {
            if (g_requests[i].response_string)
                Free(g_requests[i].response_string);
        }
        g_requests[i] = {};
    }

    PlatformShutdownHttp();
}

void UpdateHttp()
{
    for (int i = 0; i < MAX_REQUESTS; i++)
    {
        HttpRequestImpl& req = g_requests[i];
        if (!req.active)
            continue;

        HttpStatus current = HttpGetStatus((HttpRequest*)&req);

        // Check for state transition to complete/error
        if (req.last_status == HttpStatus::Pending &&
            (current == HttpStatus::Complete || current == HttpStatus::Error))
        {
            req.last_status = current;

            // Fire callback
            if (req.callback)
                req.callback((HttpRequest*)&req);
        }
    }
}

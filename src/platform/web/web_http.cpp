//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Web/Emscripten HTTP implementation using Fetch API
//

#include "../../platform.h"
#include "../../internal.h"

#include <emscripten.h>
#include <emscripten/fetch.h>

constexpr int MAX_HTTP_REQUESTS = 16;

struct WebHttpRequest {
    emscripten_fetch_t* fetch;
    u8* response_data;
    u32 response_size;
    u32 generation;
    HttpStatus status;
    int status_code;
};

struct WebHttp {
    WebHttpRequest requests[MAX_HTTP_REQUESTS];
    u32 next_request_id;
};

static WebHttp g_http = {};

static u32 GetRequestIndex(const PlatformHttpHandle& handle) {
    return (u32)(handle.value & 0xFFFFFFFF);
}

static u32 GetRequestGeneration(const PlatformHttpHandle& handle) {
    return (u32)(handle.value >> 32);
}

static PlatformHttpHandle MakeHttpHandle(u32 index, u32 generation) {
    PlatformHttpHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

static WebHttpRequest* GetRequest(const PlatformHttpHandle& handle) {
    u32 index = GetRequestIndex(handle);
    u32 generation = GetRequestGeneration(handle);

    if (index >= MAX_HTTP_REQUESTS)
        return nullptr;

    WebHttpRequest& request = g_http.requests[index];
    if (request.generation != generation)
        return nullptr;

    return &request;
}

static void CleanupRequest(WebHttpRequest* request) {
    if (request->fetch) {
        emscripten_fetch_close(request->fetch);
        request->fetch = nullptr;
    }
    if (request->response_data) {
        Free(request->response_data);
        request->response_data = nullptr;
    }
    request->response_size = 0;
    request->status = HttpStatus::None;
    request->status_code = 0;
}

static void OnFetchSuccess(emscripten_fetch_t* fetch) {
    WebHttpRequest* request = (WebHttpRequest*)fetch->userData;
    if (!request) {
        emscripten_fetch_close(fetch);
        return;
    }

    request->status_code = fetch->status;

    if (fetch->numBytes > 0) {
        request->response_data = static_cast<u8 *>(Alloc(ALLOCATOR_DEFAULT, fetch->numBytes));
        memcpy(request->response_data, fetch->data, fetch->numBytes);
        request->response_size = fetch->numBytes;
    }

    request->status = HttpStatus::Complete;
    request->fetch = nullptr;

    emscripten_fetch_close(fetch);
}

static void OnFetchError(emscripten_fetch_t* fetch) {
    WebHttpRequest* request = (WebHttpRequest*)fetch->userData;
    if (!request) {
        emscripten_fetch_close(fetch);
        return;
    }

    request->status_code = fetch->status;
    request->status = HttpStatus::Error;
    request->fetch = nullptr;

    LogError("HTTP request failed: %s (status %d)", fetch->url, fetch->status);

    emscripten_fetch_close(fetch);
}

void PlatformInitHttp() {
    g_http = {};
    g_http.next_request_id = 1;

    LogInfo("Web HTTP initialized");
}

void PlatformShutdownHttp() {
    // Clean up any pending requests
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        if (g_http.requests[i].fetch) {
            CleanupRequest(&g_http.requests[i]);
        }
    }
}

static PlatformHttpHandle StartRequest(const char* url, const char* method, const void* body, u32 body_size, const char* content_type) {
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        if (g_http.requests[i].status == HttpStatus::None) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        LogError("HTTP: No free request slots");
        return PlatformHttpHandle{0};
    }

    WebHttpRequest& request = g_http.requests[slot];
    request.generation = ++g_http.next_request_id;
    request.status = HttpStatus::Pending;
    request.status_code = 0;
    request.response_data = nullptr;
    request.response_size = 0;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    strcpy(attr.requestMethod, method);
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = OnFetchSuccess;
    attr.onerror = OnFetchError;
    attr.userData = &request;

    if (body && body_size > 0) {
        // Make a copy of the body data
        attr.requestData = (const char*)body;
        attr.requestDataSize = body_size;
    }

    if (content_type) {
        const char* headers[] = {"Content-Type", content_type, nullptr};
        attr.requestHeaders = headers;
    }

    request.fetch = emscripten_fetch(&attr, url);

    if (!request.fetch) {
        request.status = HttpStatus::Error;
        return PlatformHttpHandle{0};
    }

    return MakeHttpHandle(slot, request.generation);
}

PlatformHttpHandle PlatformGetURL(const char* url) {
    return StartRequest(url, "GET", nullptr, 0, nullptr);
}

PlatformHttpHandle PlatformPostURL(const char* url, const void* body, u32 body_size, const char* content_type) {
    return StartRequest(url, "POST", body, body_size, content_type ? content_type : "application/octet-stream");
}

HttpStatus PlatformGetStatus(const PlatformHttpHandle& handle) {
    WebHttpRequest* request = GetRequest(handle);
    if (!request)
        return HttpStatus::None;

    return request->status;
}

int PlatformGetStatusCode(const PlatformHttpHandle& handle) {
    WebHttpRequest* request = GetRequest(handle);
    if (!request)
        return 0;

    return request->status_code;
}

const u8* PlatformGetResponse(const PlatformHttpHandle& handle, u32* out_size) {
    WebHttpRequest* request = GetRequest(handle);
    if (!request || request->status != HttpStatus::Complete) {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    if (out_size) *out_size = request->response_size;
    return request->response_data;
}

void PlatformCancel(const PlatformHttpHandle& handle) {
    WebHttpRequest* request = GetRequest(handle);
    if (!request)
        return;

    if (request->fetch) {
        // Note: emscripten_fetch doesn't support cancellation directly
        // We just mark it as failed and let it complete
        request->status = HttpStatus::Error;
    }
}

void PlatformFree(const PlatformHttpHandle& handle) {
    WebHttpRequest* request = GetRequest(handle);
    if (!request)
        return;

    CleanupRequest(request);
}

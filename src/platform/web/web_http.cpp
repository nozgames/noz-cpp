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
    int status_code;
    char* response_headers;
    char* body_copy;
    PlatformHttpStatus status;
};

struct WebHttp {
    WebHttpRequest requests[MAX_HTTP_REQUESTS];
    u32 next_request_id;
};

static WebHttp g_web_http = {};

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

    WebHttpRequest& request = g_web_http.requests[index];
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
    if (request->response_headers) {
        Free(request->response_headers);
        request->response_headers = nullptr;
    }
    if (request->body_copy) {
        Free(request->body_copy);
        request->body_copy = nullptr;
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

    // Capture response headers before closing fetch
    size_t headers_len = emscripten_fetch_get_response_headers_length(fetch);
    if (headers_len > 0) {
        request->response_headers = static_cast<char*>(Alloc(ALLOCATOR_DEFAULT, headers_len + 1));
        emscripten_fetch_get_response_headers(fetch, request->response_headers, headers_len + 1);
    }

    // Free body copy now that request is complete
    if (request->body_copy) {
        Free(request->body_copy);
        request->body_copy = nullptr;
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

    // Free body copy now that request is complete
    if (request->body_copy) {
        Free(request->body_copy);
        request->body_copy = nullptr;
    }

    LogError("HTTP request failed: %s (status %d)", fetch->url, fetch->status);

    emscripten_fetch_close(fetch);
}

void PlatformInitHttp(const ApplicationTraits& traits) {
    g_web_http = {};
    g_web_http.next_request_id = 1;

    LogInfo("Web HTTP initialized");
}

void PlatformUpdateHttp() {
    // Emscripten fetch uses callbacks, no polling needed
}

void PlatformShutdownHttp() {
    // Clean up any pending requests
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        if (g_web_http.requests[i].fetch) {
            CleanupRequest(&g_web_http.requests[i]);
        }
    }
}

static PlatformHttpHandle StartRequest(const char* url, const char* method, const void* body, u32 body_size, const char* content_type, const char* custom_headers) {
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        if (g_web_http.requests[i].status == HttpStatus::None) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        LogError("HTTP: No free request slots");
        return PlatformHttpHandle{0};
    }

    WebHttpRequest& request = g_web_http.requests[slot];
    request.generation = ++g_web_http.next_request_id;
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

    char* body_copy = nullptr;
    if (body && body_size > 0) {
        // Make a copy of the body data (emscripten fetch is async)
        body_copy = (char*)Alloc(ALLOCATOR_DEFAULT, body_size);
        memcpy(body_copy, body, body_size);
        attr.requestData = body_copy;
        attr.requestDataSize = body_size;
    }
    request.body_copy = body_copy;

    // Build headers array for emscripten (format: name, value, name, value, ..., nullptr)
    // Max 32 headers (64 strings + nullptr)
    const char* headers_array[65] = {};
    int header_count = 0;

    if (content_type) {
        headers_array[header_count++] = "Content-Type";
        headers_array[header_count++] = content_type;
    }

    // Parse custom headers (format: "Name: Value\r\n")
    char* headers_copy = nullptr;
    if (custom_headers && custom_headers[0]) {
        headers_copy = (char*)Alloc(ALLOCATOR_SCRATCH, strlen(custom_headers) + 1);
        strcpy(headers_copy, custom_headers);

        char* p = headers_copy;
        while (*p && header_count < 62) {
            // Find header name end
            char* colon = strchr(p, ':');
            if (!colon) break;
            *colon = '\0';
            headers_array[header_count++] = p;

            // Skip ": " and find value
            char* value = colon + 1;
            while (*value == ' ') value++;

            // Find end of line
            char* eol = value;
            while (*eol && *eol != '\r' && *eol != '\n') eol++;
            char next = *eol;
            *eol = '\0';
            headers_array[header_count++] = value;

            // Move to next line
            p = eol;
            if (next) p++;
            if (*p == '\n') p++;
        }
    }

    headers_array[header_count] = nullptr;
    if (header_count > 0) {
        attr.requestHeaders = headers_array;
    }

    request.fetch = emscripten_fetch(&attr, url);

    if (headers_copy) {
        Free(headers_copy);
    }

    if (!request.fetch) {
        request.status = HttpStatus::Error;
        return PlatformHttpHandle{0};
    }

    return MakeHttpHandle(slot, request.generation);
}

PlatformHttpHandle PlatformGetURL(const char* url) {
    return StartRequest(url, "GET", nullptr, 0, nullptr, nullptr);
}

PlatformHttpHandle PlatformPostURL(const char* url, const void* body, u32 body_size, const char* content_type, const char* headers, const char* method) {
    return StartRequest(url, method, body, body_size, content_type ? content_type : "application/octet-stream", headers);
}

PlatformHttpStatus PlatformGetStatus(const PlatformHttpHandle& handle) {
    WebHttpRequest* request = GetRequest(handle);
    if (!request)
        return PLATFORM_HTTP_STATUS_NONE;

    return request->status;
}

int PlatformGetStatusCode(const PlatformHttpHandle& handle) {
    WebHttpRequest* request = GetRequest(handle);
    if (!request)
        return 0;

    return request->status_code;
}

bool PlatformIsFromCache(const PlatformHttpHandle& handle) {
    (void)handle;
    return false;  // Web platform doesn't expose cache info
}

const u8* PlatformGetResponse(const PlatformHttpHandle& handle, u32* out_size) {
    WebHttpRequest* request = GetRequest(handle);
    if (!request || request->status != PLATFORM_HTTP_STATUS_COMPLETE) {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    if (out_size) *out_size = request->response_size;
    return request->response_data;
}

char* PlatformGetResponseHeader(const PlatformHttpHandle& handle, const char* name, Allocator* allocator) {
    WebHttpRequest* request = GetRequest(handle);
    if (!request || !request->response_headers)
        return nullptr;

    // Headers are in format "Header-Name: value\r\n"
    size_t name_len = strlen(name);
    const char* p = request->response_headers;

    while (*p) {
        // Check if this line starts with our header name (case-insensitive)
        bool match = true;
        for (size_t i = 0; i < name_len && p[i]; i++) {
            char c1 = p[i];
            char c2 = name[i];
            if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
            if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
            if (c1 != c2) {
                match = false;
                break;
            }
        }

        if (match && p[name_len] == ':') {
            // Found the header, skip ": " and extract value
            const char* value_start = p + name_len + 1;
            while (*value_start == ' ') value_start++;

            // Find end of line
            const char* value_end = value_start;
            while (*value_end && *value_end != '\r' && *value_end != '\n')
                value_end++;

            size_t value_len = value_end - value_start;
            char* result = static_cast<char*>(Alloc(allocator ? allocator : ALLOCATOR_DEFAULT, value_len + 1));
            memcpy(result, value_start, value_len);
            result[value_len] = '\0';
            return result;
        }

        // Skip to next line
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }

    return nullptr;
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

void PlatformEncodeUrl(char* out, u32 out_size, const char* input, u32 input_length) {
    if (!out || out_size == 0)
        return;

    out[0] = '\0';

    if (!input || input_length == 0)
        return;

    // Use JavaScript's encodeURIComponent
    EM_ASM({
        var input_str = UTF8ToString($0);
        var encoded = encodeURIComponent(input_str);
        stringToUTF8(encoded, $1, $2);
    }, input, out, out_size);
}

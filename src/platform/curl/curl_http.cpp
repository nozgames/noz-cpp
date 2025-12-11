//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  HTTP implementation using libcurl (async via multi interface)
//

#include "../../platform.h"

#include <curl/curl.h>

constexpr int MAX_HTTP_REQUESTS = 16;

struct CurlHttpRequest {
    CURL* easy;
    u8* response_data;
    u32 response_size;
    u32 response_capacity;
    u32 generation;
    HttpStatus status;
    int status_code;
    char error_buffer[CURL_ERROR_SIZE];
};

struct CurlHttp {
    CURLM* multi;
    CurlHttpRequest requests[MAX_HTTP_REQUESTS];
    u32 next_request_id;
    bool initialized;
};

static CurlHttp g_http = {};

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

static CurlHttpRequest* GetRequest(const PlatformHttpHandle& handle) {
    u32 index = GetRequestIndex(handle);
    u32 generation = GetRequestGeneration(handle);

    if (index >= MAX_HTTP_REQUESTS)
        return nullptr;

    CurlHttpRequest& request = g_http.requests[index];
    if (request.generation != generation)
        return nullptr;

    return &request;
}

static void CleanupRequest(CurlHttpRequest* request) {
    if (request->easy) {
        curl_multi_remove_handle(g_http.multi, request->easy);
        curl_easy_cleanup(request->easy);
        request->easy = nullptr;
    }
    if (request->response_data) {
        Free(request->response_data);
        request->response_data = nullptr;
    }
    request->response_size = 0;
    request->response_capacity = 0;
    request->status = HttpStatus::None;
    request->status_code = 0;
    request->error_buffer[0] = '\0';
}

int DebugCallback(CURL* handle, curl_infotype type, char* data, size_t size, void* userp) {
    (void)handle;
    (void)userp;

    const char* prefix = "";
    switch (type) {
        case CURLINFO_TEXT:         prefix = "* "; break;
        case CURLINFO_HEADER_IN:    prefix = "< "; break;
        case CURLINFO_HEADER_OUT:   prefix = "> "; break;
        case CURLINFO_DATA_IN:      return 0; // Skip data logging
        case CURLINFO_DATA_OUT:     return 0;
        case CURLINFO_SSL_DATA_IN:  return 0;
        case CURLINFO_SSL_DATA_OUT: return 0;
        default: return 0;
    }

    // Remove trailing newlines for cleaner output
    while (size > 0 && (data[size-1] == '\n' || data[size-1] == '\r'))
        size--;

    if (size > 0) {
        char buffer[1024];
        size_t copy_size = size < sizeof(buffer) - 1 ? size : sizeof(buffer) - 1;
        memcpy(buffer, data, copy_size);
        buffer[copy_size] = '\0';
        LogInfo("CURL %s%s", prefix, buffer);
    }

    return 0;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    CurlHttpRequest* request = (CurlHttpRequest*)userp;
    size_t real_size = size * nmemb;

    // Grow buffer if needed
    u32 new_size = request->response_size + (u32)real_size;
    if (new_size > request->response_capacity) {
        u32 new_capacity = Max(request->response_capacity * 2, new_size + 4096);
        u8* new_data = (u8*)Alloc(ALLOCATOR_DEFAULT, new_capacity);
        if (request->response_data) {
            memcpy(new_data, request->response_data, request->response_size);
            Free(request->response_data);
        }
        request->response_data = new_data;
        request->response_capacity = new_capacity;
    }

    memcpy(request->response_data + request->response_size, contents, real_size);
    request->response_size += (u32)real_size;

    return real_size;
}

void PlatformInitHttp() {
    g_http = {};

    curl_global_init(CURL_GLOBAL_DEFAULT);

    g_http.multi = curl_multi_init();
    if (!g_http.multi) {
        LogError("Failed to initialize curl multi handle");
        return;
    }

    g_http.next_request_id = 1;
    g_http.initialized = true;

    // Log curl version info
    curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
    LogInfo("curl HTTP initialized: %s, SSL: %s", info->version, info->ssl_version ? info->ssl_version : "none");
}

void PlatformShutdownHttp() {
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        CleanupRequest(&g_http.requests[i]);
    }

    if (g_http.multi) {
        curl_multi_cleanup(g_http.multi);
        g_http.multi = nullptr;
    }

    curl_global_cleanup();
    g_http.initialized = false;
}

static CurlHttpRequest* AllocRequest() {
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        if (g_http.requests[i].status == HttpStatus::None) {
            CurlHttpRequest& req = g_http.requests[i];
            req = {};
            req.generation = ++g_http.next_request_id;
            req.status = HttpStatus::Pending;
            return &req;
        }
    }
    LogWarning("No free HTTP request slots");
    return nullptr;
}

static int GetRequestSlot(CurlHttpRequest* request) {
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        if (&g_http.requests[i] == request)
            return i;
    }
    return -1;
}

PlatformHttpHandle PlatformGetURL(const char* url) {
    if (!g_http.initialized || !g_http.multi)
        return MakeHttpHandle(0, 0xFFFFFFFF);

    CurlHttpRequest* request = AllocRequest();
    if (!request)
        return MakeHttpHandle(0, 0xFFFFFFFF);

    int slot = GetRequestSlot(request);

    request->easy = curl_easy_init();
    if (!request->easy) {
        request->status = HttpStatus::None;
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    curl_easy_setopt(request->easy, CURLOPT_URL, url);
    curl_easy_setopt(request->easy, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(request->easy, CURLOPT_WRITEDATA, request);
    curl_easy_setopt(request->easy, CURLOPT_ERRORBUFFER, request->error_buffer);
    curl_easy_setopt(request->easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(request->easy, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(request->easy, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(request->easy, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    curl_easy_setopt(request->easy, CURLOPT_PRIVATE, request);

    // SSL options - disable verification to avoid revocation check failures
    curl_easy_setopt(request->easy, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(request->easy, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(request->easy, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE | CURLSSLOPT_REVOKE_BEST_EFFORT);

    // Force IPv4 (IPv6 DNS can cause issues)
    curl_easy_setopt(request->easy, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);


    CURLMcode mc = curl_multi_add_handle(g_http.multi, request->easy);
    if (mc != CURLM_OK) {
        LogError("curl_multi_add_handle failed: %s", curl_multi_strerror(mc));
        curl_easy_cleanup(request->easy);
        request->easy = nullptr;
        request->status = HttpStatus::None;
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    LogInfo("HTTP GET: %s", url);

    return MakeHttpHandle(slot, request->generation);
}

PlatformHttpHandle PlatformPostURL(const char* url, const void* body, u32 body_size, const char* content_type) {
    if (!g_http.initialized || !g_http.multi)
        return MakeHttpHandle(0, 0xFFFFFFFF);

    CurlHttpRequest* request = AllocRequest();
    if (!request)
        return MakeHttpHandle(0, 0xFFFFFFFF);

    int slot = GetRequestSlot(request);

    request->easy = curl_easy_init();
    if (!request->easy) {
        request->status = HttpStatus::None;
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    curl_easy_setopt(request->easy, CURLOPT_URL, url);
    curl_easy_setopt(request->easy, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(request->easy, CURLOPT_WRITEDATA, request);
    curl_easy_setopt(request->easy, CURLOPT_ERRORBUFFER, request->error_buffer);
    curl_easy_setopt(request->easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(request->easy, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(request->easy, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(request->easy, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    curl_easy_setopt(request->easy, CURLOPT_PRIVATE, request);

    // POST data
    curl_easy_setopt(request->easy, CURLOPT_POST, 1L);
    curl_easy_setopt(request->easy, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(request->easy, CURLOPT_POSTFIELDSIZE, (long)body_size);

    // Content-Type header
    if (content_type) {
        struct curl_slist* headers = nullptr;
        char header[256];
        snprintf(header, sizeof(header), "Content-Type: %s", content_type);
        headers = curl_slist_append(headers, header);
        curl_easy_setopt(request->easy, CURLOPT_HTTPHEADER, headers);
    }

    // SSL options - disable verification to avoid revocation check failures
    curl_easy_setopt(request->easy, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(request->easy, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(request->easy, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE | CURLSSLOPT_REVOKE_BEST_EFFORT);

    // Force IPv4
    curl_easy_setopt(request->easy, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    CURLMcode mc = curl_multi_add_handle(g_http.multi, request->easy);
    if (mc != CURLM_OK) {
        LogError("curl_multi_add_handle failed: %s", curl_multi_strerror(mc));
        curl_easy_cleanup(request->easy);
        request->easy = nullptr;
        request->status = HttpStatus::None;
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    LogInfo("HTTP POST: %s", url);

    return MakeHttpHandle(slot, request->generation);
}

HttpStatus PlatformGetStatus(const PlatformHttpHandle& handle) {
    CurlHttpRequest* request = GetRequest(handle);
    if (!request)
        return HttpStatus::None;

    return request->status;
}

int PlatformGetStatusCode(const PlatformHttpHandle& handle) {
    CurlHttpRequest* request = GetRequest(handle);
    if (!request)
        return 0;

    return request->status_code;
}

const u8* PlatformGetResponse(const PlatformHttpHandle& handle, u32* out_size) {
    CurlHttpRequest* request = GetRequest(handle);
    if (!request || request->status != HttpStatus::Complete) {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    if (out_size) *out_size = request->response_size;
    return request->response_data;
}

void PlatformCancel(const PlatformHttpHandle& handle) {
    CurlHttpRequest* request = GetRequest(handle);
    if (!request)
        return;

    CleanupRequest(request);
}

void PlatformFree(const PlatformHttpHandle& handle) {
    CurlHttpRequest* request = GetRequest(handle);
    if (!request)
        return;

    CleanupRequest(request);
}

void PlatformUpdateHttp() {
    if (!g_http.initialized || !g_http.multi)
        return;

    int still_running = 0;

    // Perform transfers - non-blocking
    curl_multi_perform(g_http.multi, &still_running);

    // Poll for socket activity without blocking (0ms timeout)
    if (still_running > 0) {
        int numfds = 0;
        curl_multi_poll(g_http.multi, nullptr, 0, 0, &numfds);
    }

    // Check for completed transfers
    CURLMsg* msg;
    int msgs_left;
    while ((msg = curl_multi_info_read(g_http.multi, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL* easy = msg->easy_handle;
            CurlHttpRequest* request = nullptr;
            curl_easy_getinfo(easy, CURLINFO_PRIVATE, &request);

            if (request) {
                if (msg->data.result == CURLE_OK) {
                    long http_code = 0;
                    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &http_code);
                    request->status_code = (int)http_code;
                    request->status = HttpStatus::Complete;
                } else {
                    LogError("HTTP request failed: %s", request->error_buffer[0] ? request->error_buffer : curl_easy_strerror(msg->data.result));
                    request->status = HttpStatus::Error;
                }

                curl_multi_remove_handle(g_http.multi, easy);
                curl_easy_cleanup(easy);
                request->easy = nullptr;
            }
        }
    }
}

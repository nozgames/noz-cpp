//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  HTTP implementation using libcurl (single-threaded, non-blocking)
//

#include "../../platform.h"
#include "../../internal.h"

#include <curl/curl.h>

constexpr int MAX_HTTP_REQUESTS = 64;

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
    g_http.multi = nullptr;
    g_http.next_request_id = 0;
    g_http.initialized = false;

    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        g_http.requests[i].easy = nullptr;
        g_http.requests[i].response_data = nullptr;
        g_http.requests[i].response_size = 0;
        g_http.requests[i].response_capacity = 0;
        g_http.requests[i].generation = 0;
        g_http.requests[i].status = HttpStatus::None;
        g_http.requests[i].status_code = 0;
        g_http.requests[i].error_buffer[0] = '\0';
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    g_http.multi = curl_multi_init();
    if (!g_http.multi) {
        LogError("Failed to initialize curl multi handle");
        return;
    }

    // Limit connections
    curl_multi_setopt(g_http.multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, 4L);
    curl_multi_setopt(g_http.multi, CURLMOPT_MAX_HOST_CONNECTIONS, 2L);

    g_http.next_request_id = 1;
    g_http.initialized = true;

    curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
    LogInfo("curl HTTP initialized: %s, SSL: %s", info->version, info->ssl_version ? info->ssl_version : "none");
}

void PlatformShutdownHttp() {
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        CurlHttpRequest& req = g_http.requests[i];
        if (req.easy) {
            curl_multi_remove_handle(g_http.multi, req.easy);
            curl_easy_cleanup(req.easy);
            req.easy = nullptr;
        }
        if (req.response_data) {
            Free(req.response_data);
            req.response_data = nullptr;
        }
    }

    if (g_http.multi) {
        curl_multi_cleanup(g_http.multi);
        g_http.multi = nullptr;
    }

    curl_global_cleanup();
    g_http.initialized = false;
}

PlatformHttpHandle PlatformGetURL(const char* url) {
    if (!g_http.initialized || !g_http.multi)
        return MakeHttpHandle(0, 0xFFFFFFFF);

    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++) {
        if (g_http.requests[i].status == HttpStatus::None) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        LogWarning("No free HTTP request slots");
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    CurlHttpRequest& req = g_http.requests[slot];
    req.response_data = nullptr;
    req.response_size = 0;
    req.response_capacity = 0;
    req.generation = ++g_http.next_request_id;
    req.status_code = 0;
    req.error_buffer[0] = '\0';

    req.easy = curl_easy_init();
    if (!req.easy) {
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    curl_easy_setopt(req.easy, CURLOPT_URL, url);
    curl_easy_setopt(req.easy, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(req.easy, CURLOPT_WRITEDATA, &req);
    curl_easy_setopt(req.easy, CURLOPT_ERRORBUFFER, req.error_buffer);
    curl_easy_setopt(req.easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(req.easy, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(req.easy, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(req.easy, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(req.easy, CURLOPT_PRIVATE, &req);

    // SSL options
    curl_easy_setopt(req.easy, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(req.easy, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(req.easy, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE);

    // Force IPv4
    curl_easy_setopt(req.easy, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    CURLMcode mc = curl_multi_add_handle(g_http.multi, req.easy);
    if (mc != CURLM_OK) {
        LogError("curl_multi_add_handle failed: %s", curl_multi_strerror(mc));
        curl_easy_cleanup(req.easy);
        req.easy = nullptr;
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    req.status = HttpStatus::Pending;
    LogInfo("HTTP GET: %s", url);

    return MakeHttpHandle(slot, req.generation);
}

PlatformHttpHandle PlatformPostURL(const char* url, const void* body, u32 body_size, const char* content_type, const char* headers, const char* method) {
    (void)url;
    (void)body;
    (void)body_size;
    (void)content_type;
    (void)headers;
    (void)method;
    LogError("HTTP POST not implemented");
    return MakeHttpHandle(0, 0xFFFFFFFF);
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

bool PlatformIsFromCache(const PlatformHttpHandle& handle) {
    (void)handle;
    return false;  // curl doesn't use system cache
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

char* PlatformGetResponseHeader(const PlatformHttpHandle& handle, const char* name, Allocator* allocator) {
    (void)handle;
    (void)name;
    (void)allocator;
    // TODO: Implement header capture in curl callbacks
    return nullptr;
}

void PlatformCancel(const PlatformHttpHandle& handle) {
    CurlHttpRequest* request = GetRequest(handle);
    if (!request)
        return;

    if (request->easy) {
        curl_multi_remove_handle(g_http.multi, request->easy);
        curl_easy_cleanup(request->easy);
        request->easy = nullptr;
    }
    if (request->response_data) {
        Free(request->response_data);
        request->response_data = nullptr;
    }
    request->status = HttpStatus::None;
}

void PlatformFree(const PlatformHttpHandle& handle) {
    PlatformCancel(handle);
}

// Called every frame from main thread
void PlatformUpdateHttp() {
    if (!g_http.initialized || !g_http.multi)
        return;

    // Perform non-blocking curl work
    int still_running = 0;
    curl_multi_perform(g_http.multi, &still_running);

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
                    LogInfo("HTTP complete: %d (%u bytes)", (int)http_code, request->response_size);
                } else {
                    LogError("HTTP failed: %s", request->error_buffer[0] ? request->error_buffer : curl_easy_strerror(msg->data.result));
                    request->status = HttpStatus::Error;
                }

                curl_multi_remove_handle(g_http.multi, easy);
                curl_easy_cleanup(easy);
                request->easy = nullptr;
            }
        }
    }
}

void PlatformEncodeUrl(char* out, u32 out_size, const char* input, u32 input_length) {
    if (!out || out_size == 0)
        return;

    out[0] = '\0';

    if (!input || input_length == 0)
        return;

    // Create a temporary null-terminated string for curl
    char* temp = (char*)Alloc(ALLOCATOR_SCRATCH, input_length + 1);
    memcpy(temp, input, input_length);
    temp[input_length] = '\0';

    // Use curl_easy_escape to encode the URL
    CURL* curl = curl_easy_init();
    if (curl) {
        char* encoded = curl_easy_escape(curl, temp, input_length);
        if (encoded) {
            strncpy(out, encoded, out_size - 1);
            out[out_size - 1] = '\0';
            curl_free(encoded);
        }
        curl_easy_cleanup(curl);
    }

    Free(temp);
}

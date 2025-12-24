//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#include <atomic>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shlwapi.lib")

// These may not be defined in older SDKs
#ifndef HTTP_QUERY_REQUEST_FLAGS
#define HTTP_QUERY_REQUEST_FLAGS 56
#endif
#ifndef INTERNET_REQFLAG_FROM_CACHE
#define INTERNET_REQFLAG_FROM_CACHE 0x00000001
#endif

enum HttpAsyncState : u8 {
    HTTP_ASYNC_IDLE,
    HTTP_ASYNC_CONNECTING,
    HTTP_ASYNC_QUERY_DATA,
    HTTP_ASYNC_READING,
};

struct WindowsHttpRequest {
    HINTERNET connection;
    HINTERNET request;
    Stream* response;
    u8* request_body;
    u32 request_body_size;
    u32 generation;
    PlatformHttpStatus status;
    int status_code;
    std::atomic<bool> async_complete{false};
    std::atomic<DWORD_PTR> async_result{0};  // Can hold HINTERNET or byte count
    std::atomic<DWORD> async_error{0};
    HttpAsyncState async_state;
    bool headers_received;
    bool from_cache;
    DWORD bytes_available;
    DWORD bytes_read;

    // Persistent buffer for async reads
    u8 read_buffer[8192];
    DWORD read_buffer_bytes;  // Bytes read into buffer (filled by InternetReadFile)
};

struct WindowsHttp {
    HINTERNET session;
    CRITICAL_SECTION cs;
    WindowsHttpRequest* requests;
    u32 max_requests;
    u32 next_request_id;
};

static WindowsHttp g_windows_http = {};

inline u32 GetRequestIndex(const PlatformHttpHandle& handle) {
    return static_cast<u32>(handle.value & 0xFFFFFFFF);
}

inline u32 GetRequestGeneration(const PlatformHttpHandle& handle) {
    return static_cast<u32>(handle.value >> 32);
}

inline PlatformHttpHandle MakeHttpHandle(u32 index, u32 generation) {
    PlatformHttpHandle handle;
    handle.value = (static_cast<u64>(generation) << 32) | static_cast<u64>(index);
    return handle;
}

static WindowsHttpRequest* GetRequest(const PlatformHttpHandle& handle) {
    u32 index = GetRequestIndex(handle);
    u32 generation = GetRequestGeneration(handle);
    if (index >= g_windows_http.max_requests)
        return nullptr;

    WindowsHttpRequest& request = g_windows_http.requests[index];
    if (request.generation != generation)
        return nullptr;

    return &request;
}

static void CleanupRequest(WindowsHttpRequest* request) {
    if (request->request)
        InternetCloseHandle(request->request);
    if (request->connection)
        InternetCloseHandle(request->connection);

    Free(request->response);

    if (request->request_body)
        Free(request->request_body);

    request->connection = nullptr;
    request->request = nullptr;
    request->request_body = nullptr;
    request->response = nullptr;
    request->request_body_size = 0;
    request->status = PLATFORM_HTTP_STATUS_NONE;
    request->status_code = 0;
    request->async_complete.store(false);
    request->async_result.store(0);
    request->async_error.store(0);
    request->async_state = HTTP_ASYNC_IDLE;
    request->headers_received = false;
    request->from_cache = false;
    request->bytes_read = 0;
    request->bytes_available = 0;
    request->read_buffer_bytes = 0;
}

// Pack index and generation into callback context
inline DWORD_PTR MakeCallbackContext(u32 index, u32 generation) {
    return (static_cast<DWORD_PTR>(generation) << 32) | static_cast<DWORD_PTR>(index);
}

inline u32 GetCallbackIndex(DWORD_PTR context) {
    return static_cast<u32>(context & 0xFFFFFFFF);
}

inline u32 GetCallbackGeneration(DWORD_PTR context) {
    return static_cast<u32>(context >> 32);
}

static void CALLBACK InternetCallback(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength) {
    (void)hInternet;
    (void)dwStatusInformationLength;

    // Only handle REQUEST_COMPLETE
    if (dwInternetStatus != INTERNET_STATUS_REQUEST_COMPLETE)
        return;

    u32 index = GetCallbackIndex(dwContext);
    u32 generation = GetCallbackGeneration(dwContext);

    if (index >= g_windows_http.max_requests)
        return;

    WindowsHttpRequest* request = &g_windows_http.requests[index];

    // Check generation with relaxed ordering - just a quick check
    if (request->generation != generation)
        return;

    INTERNET_ASYNC_RESULT* result = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;

    // Only write to atomics from callback - main thread reads them
    request->async_error.store(result->dwError, std::memory_order_relaxed);
    request->async_result.store(result->dwResult, std::memory_order_relaxed);
    request->async_complete.store(true, std::memory_order_release);
}

void PlatformInitHttp(const ApplicationTraits& traits) {
    g_windows_http.session = nullptr;
    g_windows_http.next_request_id = 0;
    g_windows_http.requests = new WindowsHttpRequest[traits.http.max_concurrent_requests];
    g_windows_http.max_requests = static_cast<u32>(traits.http.max_concurrent_requests);

    for (u32 request_index=0; request_index < g_windows_http.max_requests; request_index++) {
        WindowsHttpRequest& reqeust = g_windows_http.requests[request_index];
        reqeust.connection = nullptr;
        reqeust.request = nullptr;
        reqeust.response = nullptr;
        reqeust.request_body = nullptr;
        reqeust.request_body_size = 0;
        reqeust.generation = 0;
        reqeust.status = PLATFORM_HTTP_STATUS_NONE;
        reqeust.status_code = 0;
        reqeust.async_complete.store(false);
        reqeust.async_result.store(0);
        reqeust.async_error.store(0);
        reqeust.async_state = HTTP_ASYNC_IDLE;
        reqeust.headers_received = false;
        reqeust.from_cache = false;
        reqeust.bytes_available = 0;
        reqeust.bytes_read = 0;
        reqeust.read_buffer_bytes = 0;
    }

    InitializeCriticalSection(&g_windows_http.cs);

    g_windows_http.session = InternetOpenW(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr,
        nullptr,
        INTERNET_FLAG_ASYNC);

    if (!g_windows_http.session)
    {
        LogError("Failed to initialize WinINet session: %lu", GetLastError());
        return;
    }

    // Set callback for async operations
    InternetSetStatusCallback(g_windows_http.session, InternetCallback);

    // Set timeouts
    DWORD timeout = 30000;
    InternetSetOptionW(g_windows_http.session, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(g_windows_http.session, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(g_windows_http.session, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
}

static void ProcessReuqest(WindowsHttpRequest& request) {
    // State machine for async operations
    switch (request.async_state) {
        case HTTP_ASYNC_CONNECTING:
        {
            // Waiting for connection to complete
            if (!request.async_complete.load(std::memory_order_acquire))
                return;

            // Check for error
            DWORD error = request.async_error.load(std::memory_order_relaxed);
            if (error >= INTERNET_ERROR_BASE) {
                request.status = PLATFORM_HTTP_STATUS_ERROR;
                return;
            }

            // Store handle if we got it from callback
            if (!request.request) {
                request.request = reinterpret_cast<HINTERNET>(request.async_result.load(std::memory_order_relaxed));
            }

            // Reset for next operation
            request.async_complete.store(false, std::memory_order_relaxed);
            request.async_state = HTTP_ASYNC_IDLE;
            break;
        }

        case HTTP_ASYNC_QUERY_DATA:
        {
            // Waiting for InternetQueryDataAvailable to complete
            if (!request.async_complete.load(std::memory_order_acquire))
                return;

            DWORD error = request.async_error.load(std::memory_order_relaxed);
            if (error >= INTERNET_ERROR_BASE) {
                request.status = PLATFORM_HTTP_STATUS_ERROR;
                return;
            }

            // Result contains bytes available
            request.bytes_available = static_cast<DWORD>(request.async_result.load(std::memory_order_relaxed));
            request.async_complete.store(false, std::memory_order_relaxed);
            request.async_state = HTTP_ASYNC_IDLE;

            if (request.bytes_available == 0) {
                // EOF
                if (request.response)
                    SeekBegin(request.response, 0);
                request.status = PLATFORM_HTTP_STATUS_COMPLETE;
                return;
            }
            break;
        }

        case HTTP_ASYNC_READING:
        {
            // Waiting for InternetReadFile to complete
            if (!request.async_complete.load(std::memory_order_acquire))
                return;

            DWORD error = request.async_error.load(std::memory_order_relaxed);
            if (error >= INTERNET_ERROR_BASE) {
                request.status = PLATFORM_HTTP_STATUS_ERROR;
                return;
            }

            request.async_complete.store(false, std::memory_order_relaxed);
            request.async_state = HTTP_ASYNC_IDLE;

            // read_buffer_bytes was filled by InternetReadFile
            if (request.read_buffer_bytes == 0) {
                // EOF
                if (request.response)
                    SeekBegin(request.response, 0);
                request.status = PLATFORM_HTTP_STATUS_COMPLETE;
                return;
            }

            WriteBytes(request.response, request.read_buffer, request.read_buffer_bytes);
            request.bytes_read += request.read_buffer_bytes;
            request.read_buffer_bytes = 0;
            break;
        }

        case HTTP_ASYNC_IDLE:
            break;
    }

    // Process headers if we haven't yet
    if (!request.headers_received && request.request) {
        DWORD status_code = 0;
        DWORD size;

        DWORD content_length = 0;
        size = sizeof(content_length);
        if (HttpQueryInfoW(
            request.request,
            HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
            &content_length,
            &size,
            nullptr))
        {
            if (content_length > 0) {
                request.bytes_available = content_length;
                if (request.response == nullptr) {
                    request.response = CreateStream(ALLOCATOR_DEFAULT, content_length + 1);
                    GetData(request.response)[content_length] = 0;
                }
                else
                    Resize(request.response, content_length);
            }
        }

        size = sizeof(status_code);
        if (HttpQueryInfoW(
            request.request,
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            &status_code,
            &size,
            nullptr))
        {
            request.status_code = static_cast<int>(status_code);
            request.headers_received = true;

            DWORD req_flags = 0;
            size = sizeof(req_flags);
            if (InternetQueryOptionW(
                request.request,
                INTERNET_OPTION_REQUEST_FLAGS,
                &req_flags,
                &size))
            {
                request.from_cache = (req_flags & INTERNET_REQFLAG_FROM_CACHE) != 0;
            }
        } else {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING)
                return;
            if (err != ERROR_HTTP_HEADER_NOT_FOUND)
                return;
        }
    }

    // Create stream if needed
    if (!request.response) {
        u32 initial_cap = request.bytes_available > 0 ? request.bytes_available : 8192u;
        request.response = CreateStream(ALLOCATOR_DEFAULT, initial_cap);
    }

    // Start async read - use persistent variable for byte count
    request.read_buffer_bytes = 0;
    if (!InternetReadFile(
        request.request,
        request.read_buffer,
        sizeof(request.read_buffer),
        &request.read_buffer_bytes))
    {
        DWORD err = GetLastError();
        if (err == ERROR_IO_PENDING) {
            request.async_state = HTTP_ASYNC_READING;
            return;
        }
        request.status = PLATFORM_HTTP_STATUS_ERROR;
        return;
    }

    // Completed synchronously
    if (request.read_buffer_bytes == 0) {
        if (request.response)
            SeekBegin(request.response, 0);
        request.status = PLATFORM_HTTP_STATUS_COMPLETE;
        return;
    }

    WriteBytes(request.response, request.read_buffer, request.read_buffer_bytes);
    request.bytes_read += request.read_buffer_bytes;
    request.read_buffer_bytes = 0;
}

void PlatformUpdateHttp() {
    for (u32 request_index = 0; request_index < g_windows_http.max_requests; request_index++) {
        WindowsHttpRequest& request = g_windows_http.requests[request_index];
        if (request.status != PLATFORM_HTTP_STATUS_PENDING)
            continue;

        // Need either a handle or be waiting for one
        if (!request.request && request.async_state != HTTP_ASYNC_CONNECTING)
            continue;

        ProcessReuqest(request);
    }
}

void PlatformShutdownHttp() {
    for (u32 request_index=0; request_index < g_windows_http.max_requests; request_index++) {
        CleanupRequest(&g_windows_http.requests[request_index]);
    }

    if (g_windows_http.session)
    {
        InternetCloseHandle(g_windows_http.session);
        g_windows_http.session = nullptr;
    }

    DeleteCriticalSection(&g_windows_http.cs);
}

static WindowsHttpRequest* AllocRequest(u32* out_index) {
    for (u32 request_index=0; request_index<g_windows_http.max_requests; request_index++) {
        if (g_windows_http.requests[request_index].status == PLATFORM_HTTP_STATUS_NONE ||
            g_windows_http.requests[request_index].status == PLATFORM_HTTP_STATUS_COMPLETE ||
            g_windows_http.requests[request_index].status == PLATFORM_HTTP_STATUS_ERROR) {
            CleanupRequest(&g_windows_http.requests[request_index]);
            *out_index = request_index;
            return &g_windows_http.requests[request_index];
        }
    }
    return nullptr;
}

PlatformHttpHandle PlatformGetURL(const char* url)
{
    if (!g_windows_http.session)
        return MakeHttpHandle(0, 0xFFFFFFFF);

    u32 request_index = 0;
    WindowsHttpRequest* request = AllocRequest(&request_index);
    if (!request)
    {
        LogWarning("No free HTTP request slots");
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Convert URL to wide string
    int url_len = (int)strlen(url);
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, url, url_len, nullptr, 0);
    wchar_t* wide_url = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, (wide_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, url, url_len, wide_url, wide_len);
    wide_url[wide_len] = 0;

    // Determine flags based on URL scheme
    DWORD flags = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_UI;
    if (_strnicmp(url, "https://", 8) == 0)
        flags |= INTERNET_FLAG_SECURE;

    // Set generation and status BEFORE making async call so callback can validate
    request->generation = ++g_windows_http.next_request_id;
    request->status = PLATFORM_HTTP_STATUS_PENDING;

    // Open URL - WinINet handles connection pooling and caching automatically
    request->request = InternetOpenUrlW(
        g_windows_http.session,
        wide_url,
        nullptr,  // headers
        0,        // headers length
        flags,
        MakeCallbackContext(request_index, request->generation));

    Free(wide_url);

    if (!request->request)
    {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING)
        {
            LogError("InternetOpenUrl failed: %lu (0x%08lX)", err, err);
            request->status = PLATFORM_HTTP_STATUS_NONE;
            return MakeHttpHandle(0, 0xFFFFFFFF);
        }
        // Handle will come through callback
        request->async_state = HTTP_ASYNC_CONNECTING;
    }
    else
    {
        // Got handle immediately - ready to read
        request->async_state = HTTP_ASYNC_IDLE;
    }

    return MakeHttpHandle(request_index, request->generation);
}

PlatformHttpHandle PlatformPostURL(const char* url, const void* body, u32 body_size, const char* content_type, const char* headers, const char* method) {
    if (!g_windows_http.session)
        return MakeHttpHandle(0, 0xFFFFFFFF);

    u32 request_index = 0;
    WindowsHttpRequest* request = AllocRequest(&request_index);
    if (!request)
    {
        LogWarning("No free HTTP request slots");
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Parse URL
    URL_COMPONENTSW url_components = {};
    url_components.dwStructSize = sizeof(url_components);
    url_components.dwSchemeLength = (DWORD)-1;
    url_components.dwHostNameLength = (DWORD)-1;
    url_components.dwUrlPathLength = (DWORD)-1;
    url_components.dwExtraInfoLength = (DWORD)-1;

    int url_len = (int)strlen(url);
    int w_url_len = MultiByteToWideChar(CP_UTF8, 0, url, url_len, nullptr, 0);
    wchar_t* w_url = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, (w_url_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, url, url_len, w_url, w_url_len);
    w_url[w_url_len] = 0;

    if (!InternetCrackUrlW(w_url, w_url_len, 0, &url_components))
    {
        Free(w_url);
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Extract hostname
    wchar_t hostname[256] = {};
    wcsncpy_s(hostname, url_components.lpszHostName, url_components.dwHostNameLength);

    // Set generation and status BEFORE making async calls so callback can validate
    request->generation = ++g_windows_http.next_request_id;
    request->status = PLATFORM_HTTP_STATUS_PENDING;
    DWORD_PTR callback_context = MakeCallbackContext(request_index, request->generation);

    // Connect
    HINTERNET connection = InternetConnectW(
        g_windows_http.session,
        hostname,
        url_components.nPort,
        nullptr,
        nullptr,
        INTERNET_SERVICE_HTTP,
        0,
        callback_context);

    if (!connection)
    {
        Free(w_url);
        request->status = PLATFORM_HTTP_STATUS_NONE;
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Build path with query string
    int path_len = url_components.dwUrlPathLength + url_components.dwExtraInfoLength + 1;
    wchar_t* w_path = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, path_len * sizeof(wchar_t));
    wcsncpy_s(w_path, path_len, url_components.lpszUrlPath, url_components.dwUrlPathLength);
    if (url_components.dwExtraInfoLength > 0 && url_components.lpszExtraInfo)
    {
        wcsncat_s(w_path, path_len, url_components.lpszExtraInfo, url_components.dwExtraInfoLength);
    }

    // Convert method to wide
    int method_len = (int)strlen(method);
    int w_method_len = MultiByteToWideChar(CP_UTF8, 0, method, method_len, nullptr, 0);
    wchar_t* w_method = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, (w_method_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, method, method_len, w_method, w_method_len);
    w_method[w_method_len] = 0;

    // Create request
    DWORD flags = INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_UI | INTERNET_FLAG_NO_CACHE_WRITE;
    if (url_components.nScheme == INTERNET_SCHEME_HTTPS)
        flags |= INTERNET_FLAG_SECURE;

    request->request = HttpOpenRequestW(
        connection,
        w_method,
        w_path,
        nullptr,
        nullptr,
        nullptr,
        flags,
        callback_context);

    Free(w_url);
    Free(w_path);
    Free(w_method);

    if (!request->request)
    {
        InternetCloseHandle(connection);
        request->status = PLATFORM_HTTP_STATUS_NONE;
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Store connection handle - must stay open until request completes
    request->connection = connection;

    // Build headers
    wchar_t w_headers[1024] = {};
    if (content_type)
        swprintf_s(w_headers, L"Content-Type: %hs\r\n", content_type);
    else
        wcscpy_s(w_headers, L"Content-Type: application/octet-stream\r\n");

    if (headers && headers[0])
    {
        int h_len = MultiByteToWideChar(CP_UTF8, 0, headers, -1, nullptr, 0);
        wchar_t* w_custom = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, h_len * sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, headers, -1, w_custom, h_len);
        wcscat_s(w_headers, w_custom);
        Free(w_custom);
    }

    // Copy body for async send
    if (body && body_size > 0)
    {
        request->request_body = (u8*)Alloc(ALLOCATOR_DEFAULT, body_size);
        memcpy(request->request_body, body, body_size);
        request->request_body_size = body_size;
    }

    // Send request
    if (!HttpSendRequestW(
        request->request,
        w_headers,
        (DWORD)wcslen(w_headers),
        request->request_body,
        request->request_body_size))
    {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING)
        {
            LogError("HttpSendRequest failed: %lu (0x%08lX)", err, err);
            request->status = PLATFORM_HTTP_STATUS_ERROR;
        }
        else
        {
            // Wait for callback
            request->async_state = HTTP_ASYNC_CONNECTING;
        }
    }
    else
    {
        // Send completed immediately
        request->async_state = HTTP_ASYNC_IDLE;
    }

    return MakeHttpHandle(request_index, request->generation);
}

PlatformHttpStatus PlatformGetStatus(const PlatformHttpHandle& handle) {
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request) return PLATFORM_HTTP_STATUS_NONE;
    return request->status;
}

int PlatformGetStatusCode(const PlatformHttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request)
        return 0;

    return request->status_code;
}

bool PlatformIsFromCache(const PlatformHttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request)
        return false;

    return request->from_cache;
}

Stream* PlatformReleaseResponseStream(const PlatformHttpHandle& handle) {
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request || request->status != PLATFORM_HTTP_STATUS_COMPLETE) {
        return nullptr;
    }

    Stream* stream = request->response;
    request->response = nullptr;
    return stream;
}

char* PlatformGetResponseHeader(const PlatformHttpHandle& handle, const char* name, Allocator* allocator)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request || !request->request)
        return nullptr;

    // Convert header name to wide string
    int name_len = (int)strlen(name);
    int w_name_len = MultiByteToWideChar(CP_UTF8, 0, name, name_len, nullptr, 0);
    wchar_t* w_name = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, (w_name_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, name, name_len, w_name, w_name_len);
    w_name[w_name_len] = 0;

    // Query header size
    DWORD size = 0;
    DWORD index = 0;
    HttpQueryInfoW(request->request, HTTP_QUERY_CUSTOM, w_name, &size, &index);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0)
    {
        Free(w_name);
        return nullptr;
    }

    // Get header value
    wchar_t* w_value = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, size);
    index = 0;
    if (!HttpQueryInfoW(request->request, HTTP_QUERY_CUSTOM, w_name, &size, &index))
    {
        Free(w_name);
        Free(w_value);
        return nullptr;
    }

    Free(w_name);

    // Convert to UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, w_value, -1, nullptr, 0, nullptr, nullptr);
    char* result = (char*)Alloc(allocator ? allocator : ALLOCATOR_DEFAULT, utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, w_value, -1, result, utf8_len, nullptr, nullptr);

    Free(w_value);
    return result;
}

void PlatformCancel(const PlatformHttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request)
        return;

    CleanupRequest(request);
}

void PlatformFree(const PlatformHttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request) {
        return;
    }

    CleanupRequest(request);
    request->generation++;
}

void PlatformEncodeUrl(char* out, u32 out_size, const char* input, u32 input_length)
{
    if (!out || out_size == 0)
        return;

    out[0] = '\0';

    if (!input || input_length == 0)
        return;

    // Convert input to wide string
    int w_len = MultiByteToWideChar(CP_UTF8, 0, input, input_length, nullptr, 0);
    wchar_t* w_input = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, (w_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, input, input_length, w_input, w_len);
    w_input[w_len] = 0;

    // Encode
    DWORD encoded_len = out_size;
    wchar_t* w_output = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, out_size * sizeof(wchar_t));

    if (SUCCEEDED(UrlEscapeW(w_input, w_output, &encoded_len, URL_ESCAPE_SEGMENT_ONLY | URL_ESCAPE_PERCENT)))
    {
        WideCharToMultiByte(CP_UTF8, 0, w_output, -1, out, out_size, nullptr, nullptr);
    }
    else
    {
        WideCharToMultiByte(CP_UTF8, 0, w_input, -1, out, out_size, nullptr, nullptr);
    }

    Free(w_input);
    Free(w_output);
}

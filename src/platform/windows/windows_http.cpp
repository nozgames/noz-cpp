//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <shlwapi.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "shlwapi.lib")

constexpr int MAX_HTTP_REQUESTS = 16;

struct WindowsHttpRequest
{
    HINTERNET connection;
    HINTERNET request;
    u8* response_data;
    u32 response_size;
    u32 response_capacity;
    u8* request_body;       // Copy of request body for async send
    u32 request_body_size;
    u32 generation;
    HttpStatus status;
    int status_code;
};

struct WindowsHttp
{
    HINTERNET session;
    WindowsHttpRequest requests[MAX_HTTP_REQUESTS];
    u32 next_request_id;
};

static WindowsHttp g_http = {};

static u32 GetRequestIndex(const PlatformHttpHandle& handle)
{
    return (u32)(handle.value & 0xFFFFFFFF);
}

static u32 GetRequestGeneration(const PlatformHttpHandle& handle)
{
    return (u32)(handle.value >> 32);
}

static PlatformHttpHandle MakeHttpHandle(u32 index, u32 generation)
{
    PlatformHttpHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

static WindowsHttpRequest* GetRequest(const PlatformHttpHandle& handle)
{
    u32 index = GetRequestIndex(handle);
    u32 generation = GetRequestGeneration(handle);

    if (index >= MAX_HTTP_REQUESTS)
        return nullptr;

    WindowsHttpRequest& request = g_http.requests[index];
    if (request.generation != generation)
        return nullptr;

    return &request;
}

static void CleanupRequest(WindowsHttpRequest* request)
{
    if (request->request)
    {
        WinHttpCloseHandle(request->request);
        request->request = nullptr;
    }
    if (request->connection)
    {
        WinHttpCloseHandle(request->connection);
        request->connection = nullptr;
    }
    if (request->response_data)
    {
        Free(request->response_data);
        request->response_data = nullptr;
    }
    if (request->request_body)
    {
        Free(request->request_body);
        request->request_body = nullptr;
    }
    request->response_size = 0;
    request->response_capacity = 0;
    request->request_body_size = 0;
    request->status = HttpStatus::None;
    request->status_code = 0;
}

void PlatformInitHttp()
{
    g_http = {};

    g_http.session = WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC);

    if (!g_http.session)
    {
        LogError("Failed to initialize WinHTTP session");
        return;
    }

    // Enable redirect following
    DWORD option = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(g_http.session, WINHTTP_OPTION_REDIRECT_POLICY, &option, sizeof(option));

    // Enable TLS 1.2/1.3
    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
    WinHttpSetOption(g_http.session, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

    // Enable automatic gzip/deflate decompression (Windows 8.1+)
    DWORD decompression = WINHTTP_DECOMPRESSION_FLAG_ALL;
    WinHttpSetOption(g_http.session, WINHTTP_OPTION_DECOMPRESSION, &decompression, sizeof(decompression));

    // Set timeouts (connect, send, receive, resolve) in milliseconds
    WinHttpSetTimeouts(g_http.session, 10000, 10000, 30000, 10000);
}

void PlatformUpdateHttp()
{
    // WinHTTP uses async callbacks, no polling needed
}

void PlatformShutdownHttp()
{
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++)
    {
        CleanupRequest(&g_http.requests[i]);
    }

    if (g_http.session)
    {
        WinHttpCloseHandle(g_http.session);
        g_http.session = nullptr;
    }

    g_http = {};
}

static void CALLBACK WindowHttpCallback(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength)
{
    (void)hInternet;
    (void)dwStatusInformationLength;

    WindowsHttpRequest* request = (WindowsHttpRequest*)dwContext;
    if (!request)
        return;

    switch (dwInternetStatus)
    {
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            WinHttpReceiveResponse(request->request, nullptr);
            break;
        }

        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            DWORD status_code = 0;
            DWORD size = sizeof(status_code);
            WinHttpQueryHeaders(
                request->request,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &status_code,
                &size,
                WINHTTP_NO_HEADER_INDEX);
            request->status_code = (int)status_code;

            WinHttpQueryDataAvailable(request->request, nullptr);
            break;
        }

        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
        {
            DWORD bytes_available = *((DWORD*)lpvStatusInformation);
            if (bytes_available > 0)
            {
                // Grow buffer if needed
                u32 new_size = request->response_size + bytes_available;
                if (new_size > request->response_capacity)
                {
                    u32 new_capacity = Max(request->response_capacity * 2, new_size + 4096);
                    u8* new_data = (u8*)Alloc(ALLOCATOR_DEFAULT, new_capacity);
                    if (request->response_data)
                    {
                        memcpy(new_data, request->response_data, request->response_size);
                        Free(request->response_data);
                    }
                    request->response_data = new_data;
                    request->response_capacity = new_capacity;
                }

                WinHttpReadData(
                    request->request,
                    request->response_data + request->response_size,
                    bytes_available,
                    nullptr);
            }
            else
            {
                // No more data - request complete
                request->status = HttpStatus::Complete;
            }
            break;
        }

        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            DWORD bytes_read = dwStatusInformationLength;
            if (bytes_read > 0)
            {
                request->response_size += bytes_read;
                WinHttpQueryDataAvailable(request->request, nullptr);
            }
            else
            {
                request->status = HttpStatus::Complete;
            }
            break;
        }

        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            WINHTTP_ASYNC_RESULT* result = (WINHTTP_ASYNC_RESULT*)lpvStatusInformation;
            LogError("HTTP async error: API=%lu, Error=%lu (0x%08lX)",
                     result->dwResult, result->dwError, result->dwError);
            request->status = HttpStatus::Error;
            break;
        }
    }
}

PlatformHttpHandle PlatformGetURL(const char* url)
{
    if (!g_http.session)
        return MakeHttpHandle(0, 0xFFFFFFFF);

    // Find free slot
    WindowsHttpRequest* request = nullptr;
    u32 request_index = 0;
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++)
    {
        if (g_http.requests[i].status == HttpStatus::None ||
            g_http.requests[i].status == HttpStatus::Complete ||
            g_http.requests[i].status == HttpStatus::Error)
        {
            CleanupRequest(&g_http.requests[i]);
            request = &g_http.requests[i];
            request_index = i;
            break;
        }
    }

    if (!request)
    {
        LogWarning("No free HTTP request slots");
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Parse URL
    URL_COMPONENTS url_components = {};
    url_components.dwStructSize = sizeof(url_components);
    url_components.dwSchemeLength = (DWORD)-1;
    url_components.dwHostNameLength = (DWORD)-1;
    url_components.dwUrlPathLength = (DWORD)-1;

    // Convert URL to wide string
    int url_len = (int)strlen(url);
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, url, url_len, nullptr, 0);
    wchar_t* wide_url = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, (wide_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, url, url_len, wide_url, wide_len);
    wide_url[wide_len] = 0;

    if (!WinHttpCrackUrl(wide_url, wide_len, 0, &url_components))
    {
        Free(wide_url);
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Extract hostname
    wchar_t hostname[256] = {};
    wcsncpy_s(hostname, url_components.lpszHostName, url_components.dwHostNameLength);

    LogInfo("HTTP GET: host='%ls', port=%d, path='%ls'",
            hostname, url_components.nPort, url_components.lpszUrlPath);

    // Connect
    request->connection = WinHttpConnect(
        g_http.session,
        hostname,
        url_components.nPort,
        0);

    if (!request->connection)
    {
        DWORD err = GetLastError();
        LogError("WinHttpConnect failed: %lu (0x%08lX)", err, err);
        Free(wide_url);
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Create request
    DWORD flags = (url_components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    request->request = WinHttpOpenRequest(
        request->connection,
        L"GET",
        url_components.lpszUrlPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);

    Free(wide_url);

    if (!request->request)
    {
        CleanupRequest(request);
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Ignore certificate revocation check failures (common with CDNs)
    DWORD security_flags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                           SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                           SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                           SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    WinHttpSetOption(request->request, WINHTTP_OPTION_SECURITY_FLAGS, &security_flags, sizeof(security_flags));

    // Set callback
    WinHttpSetStatusCallback(
        request->request,
        WindowHttpCallback,
        WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS,
        0);

    // Send request
    request->status = HttpStatus::Pending;
    request->generation = ++g_http.next_request_id;

    if (!WinHttpSendRequest(
        request->request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        (DWORD_PTR)request))
    {
        DWORD err = GetLastError();
        LogError("WinHttpSendRequest failed: %lu (0x%08lX)", err, err);
        request->status = HttpStatus::Error;
    }

    return MakeHttpHandle(request_index, request->generation);
}

PlatformHttpHandle PlatformPostURL(const char* url, const void* body, u32 body_size, const char* content_type, const char* headers, const char* method) {
    if (!g_http.session)
        return MakeHttpHandle(0, 0xFFFFFFFF);

    // Find free slot
    WindowsHttpRequest* request = nullptr;
    u32 request_index = 0;
    for (int i = 0; i < MAX_HTTP_REQUESTS; i++)
    {
        if (g_http.requests[i].status == HttpStatus::None ||
            g_http.requests[i].status == HttpStatus::Complete ||
            g_http.requests[i].status == HttpStatus::Error)
        {
            CleanupRequest(&g_http.requests[i]);
            request = &g_http.requests[i];
            request_index = i;
            break;
        }
    }

    if (!request)
    {
        LogWarning("No free HTTP request slots");
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Parse URL
    URL_COMPONENTS url_components = {};
    url_components.dwStructSize = sizeof(url_components);
    url_components.dwSchemeLength = (DWORD)-1;
    url_components.dwHostNameLength = (DWORD)-1;
    url_components.dwUrlPathLength = (DWORD)-1;
    url_components.dwExtraInfoLength = (DWORD)-1;  // Query string

    int url_len = (int)strlen(url);
    int w_url_len = MultiByteToWideChar(CP_UTF8, 0, url, url_len, nullptr, 0);
    wchar_t* w_url = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, (w_url_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, url, url_len, w_url, w_url_len);
    w_url[w_url_len] = 0;

    if (!WinHttpCrackUrl(w_url, w_url_len, 0, &url_components))
    {
        Free(w_url);
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    wchar_t hostname[256] = {};
    wcsncpy_s(hostname, url_components.lpszHostName, url_components.dwHostNameLength);

    request->connection = WinHttpConnect(
        g_http.session,
        hostname,
        url_components.nPort,
        0);

    if (!request->connection) {
        Free(w_url);
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Build full path with query string
    wchar_t* w_full_path = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, (url_components.dwUrlPathLength + url_components.dwExtraInfoLength + 1) * sizeof(wchar_t));
    wcsncpy_s(w_full_path, url_components.dwUrlPathLength + 1, url_components.lpszUrlPath, url_components.dwUrlPathLength);
    if (url_components.dwExtraInfoLength > 0 && url_components.lpszExtraInfo)
    {
        wcsncat_s(w_full_path, url_components.dwUrlPathLength + url_components.dwExtraInfoLength + 1,
                  url_components.lpszExtraInfo, url_components.dwExtraInfoLength);
    }

    int method_len = Length(method);
    int w_method_len = MultiByteToWideChar(CP_UTF8, 0, method, method_len, nullptr, 0);
    wchar_t* w_method = static_cast<wchar_t*>(Alloc(ALLOCATOR_SCRATCH, (w_url_len + 1) * sizeof(wchar_t)));
    MultiByteToWideChar(CP_UTF8, 0, method, method_len, w_method, w_method_len);
    w_method[w_method_len] = 0;

    DWORD flags = (url_components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    request->request = WinHttpOpenRequest(
        request->connection,
        w_method,
        w_full_path,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags);

    Free(w_url);
    Free(w_method);
    Free(w_full_path);

    if (!request->request)
    {
        CleanupRequest(request);
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    // Ignore certificate revocation check failures (common with CDNs)
    DWORD security_flags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                           SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                           SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                           SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    WinHttpSetOption(request->request, WINHTTP_OPTION_SECURITY_FLAGS, &security_flags, sizeof(security_flags));

    // Add Content-Type header
    wchar_t w_content_type[256] = {};
    if (content_type)
        swprintf_s(w_content_type, L"Content-Type: %hs", content_type);
    else
        wcscpy_s(w_content_type, L"Content-Type: application/octet-stream");

    if (!WinHttpAddRequestHeaders(request->request, w_content_type, (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE))
    {
        LogError("WinHttpAddRequestHeaders (Content-Type) failed: %lu", GetLastError());
    }

    // Disable keep-alive
    //WinHttpAddRequestHeaders(request->request, L"Connection: close", (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

    // Accept any content type
    WinHttpAddRequestHeaders(request->request, L"Accept: */*", (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);

    // Add custom headers if provided
    if (headers && headers[0])
    {
        // Parse and add headers one at a time
        char* headers_copy = (char*)Alloc(ALLOCATOR_SCRATCH, Length(headers) + 1);
        strcpy(headers_copy, headers);

        char* line = headers_copy;
        while (*line)
        {
            // Find end of line
            char* eol = line;
            while (*eol && *eol != '\r' && *eol != '\n') eol++;
            char saved = *eol;
            *eol = '\0';

            if (strlen(line) > 0)
            {
                // Convert to wide string
                int w_len = MultiByteToWideChar(CP_UTF8, 0, line, -1, nullptr, 0);
                wchar_t* w_header = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, w_len * sizeof(wchar_t));
                MultiByteToWideChar(CP_UTF8, 0, line, -1, w_header, w_len);

                if (!WinHttpAddRequestHeaders(request->request, w_header, (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD))
                {
                    LogError("WinHttpAddRequestHeaders failed for [%ls]: %lu", w_header, GetLastError());
                }
                Free(w_header);
            }

            // Move to next line
            *eol = saved;
            line = eol;
            while (*line == '\r' || *line == '\n') line++;
        }
        Free(headers_copy);
    }

    WinHttpSetStatusCallback(
        request->request,
        WindowHttpCallback,
        WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS,
        0);

    // Copy body data - must stay valid until async send completes
    if (body && body_size > 0)
    {
        request->request_body = (u8*)Alloc(ALLOCATOR_DEFAULT, body_size);
        memcpy(request->request_body, body, body_size);
        request->request_body_size = body_size;
    }

    request->status = HttpStatus::Pending;
    request->generation = ++g_http.next_request_id;

    if (!WinHttpSendRequest(
        request->request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)request->request_body,
        request->request_body_size,
        request->request_body_size,
        (DWORD_PTR)request))
    {
        request->status = HttpStatus::Error;
    }

    return MakeHttpHandle(request_index, request->generation);
}

HttpStatus PlatformGetStatus(const PlatformHttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request)
        return HttpStatus::None;

    return request->status;
}

int PlatformGetStatusCode(const PlatformHttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request)
        return 0;

    return request->status_code;
}

const u8* PlatformGetResponse(const PlatformHttpHandle& handle, u32* out_size)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request || request->status != HttpStatus::Complete)
    {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    if (out_size) *out_size = request->response_size;
    return request->response_data;
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

    // Query header size first
    DWORD size = 0;
    WinHttpQueryHeaders(
        request->request,
        WINHTTP_QUERY_CUSTOM,
        w_name,
        WINHTTP_NO_OUTPUT_BUFFER,
        &size,
        WINHTTP_NO_HEADER_INDEX);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0)
    {
        Free(w_name);
        return nullptr;
    }

    // Allocate and query the header value
    wchar_t* w_value = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, size);
    if (!WinHttpQueryHeaders(
        request->request,
        WINHTTP_QUERY_CUSTOM,
        w_name,
        w_value,
        &size,
        WINHTTP_NO_HEADER_INDEX))
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
    if (!request)
        return;

    CleanupRequest(request);
    request->generation++; // Invalidate any existing handles
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

    // Encode using WinHTTP
    DWORD encoded_len = out_size;
    wchar_t* w_output = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, out_size * sizeof(wchar_t));

    // Use UrlEscapeW with URL_ESCAPE_SEGMENT_ONLY to encode query string parts
    // This encodes special characters but preserves path structure
    if (SUCCEEDED(UrlEscapeW(w_input, w_output, &encoded_len, URL_ESCAPE_SEGMENT_ONLY | URL_ESCAPE_PERCENT)))
    {
        // Convert back to UTF-8
        WideCharToMultiByte(CP_UTF8, 0, w_output, -1, out, out_size, nullptr, nullptr);
    }
    else
    {
        // Fallback: just copy the input if encoding fails
        WideCharToMultiByte(CP_UTF8, 0, w_input, -1, out, out_size, nullptr, nullptr);
    }

    Free(w_input);
    Free(w_output);
}

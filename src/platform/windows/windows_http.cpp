//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

constexpr int MAX_HTTP_REQUESTS = 16;

struct WindowsHttpRequest
{
    HINTERNET connection;
    HINTERNET request;
    u8* response_data;
    u32 response_size;
    u32 response_capacity;
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
    request->response_size = 0;
    request->response_capacity = 0;
    request->status = HttpStatus::None;
    request->status_code = 0;
}

void PlatformInitHttp()
{
    g_http = {};

    g_http.session = WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
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

PlatformHttpHandle PlatformPostURL(const char* url, const void* body, u32 body_size, const char* content_type)
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

    wchar_t hostname[256] = {};
    wcsncpy_s(hostname, url_components.lpszHostName, url_components.dwHostNameLength);

    request->connection = WinHttpConnect(
        g_http.session,
        hostname,
        url_components.nPort,
        0);

    if (!request->connection)
    {
        Free(wide_url);
        return MakeHttpHandle(0, 0xFFFFFFFF);
    }

    DWORD flags = (url_components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    request->request = WinHttpOpenRequest(
        request->connection,
        L"POST",
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

    // Build Content-Type header
    wchar_t headers[512] = {};
    if (content_type)
    {
        swprintf_s(headers, L"Content-Type: %hs\r\n", content_type);
    }
    else
    {
        wcscpy_s(headers, L"Content-Type: application/octet-stream\r\n");
    }

    WinHttpSetStatusCallback(
        request->request,
        WindowHttpCallback,
        WINHTTP_CALLBACK_FLAG_ALL_COMPLETIONS,
        0);

    request->status = HttpStatus::Pending;
    request->generation = ++g_http.next_request_id;

    if (!WinHttpSendRequest(
        request->request,
        headers,
        (DWORD)-1,
        (LPVOID)body,
        body_size,
        body_size,
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

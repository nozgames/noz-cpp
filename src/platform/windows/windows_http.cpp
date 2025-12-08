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
    platform::HttpStatus status;
    int status_code;
};

struct WindowsHttp
{
    HINTERNET session;
    WindowsHttpRequest requests[MAX_HTTP_REQUESTS];
    u32 next_request_id;
};

static WindowsHttp g_http = {};

static u32 GetRequestIndex(const platform::HttpHandle& handle)
{
    return (u32)(handle.value & 0xFFFFFFFF);
}

static u32 GetRequestGeneration(const platform::HttpHandle& handle)
{
    return (u32)(handle.value >> 32);
}

static platform::HttpHandle MakeHttpHandle(u32 index, u32 generation)
{
    platform::HttpHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

static WindowsHttpRequest* GetRequest(const platform::HttpHandle& handle)
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
    request->status = platform::HttpStatus::None;
    request->status_code = 0;
}

void platform::InitializeHttp()
{
    g_http = {};

    g_http.session = WinHttpOpen(
        L"NoZ/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        WINHTTP_FLAG_ASYNC);

    if (!g_http.session)
    {
        LogError("Failed to initialize WinHTTP session");
        return;
    }

    // Set timeouts (connect, send, receive, resolve) in milliseconds
    WinHttpSetTimeouts(g_http.session, 10000, 10000, 30000, 10000);
}

void platform::ShutdownHttp()
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
                request->status = platform::HttpStatus::Complete;
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
                request->status = platform::HttpStatus::Complete;
            }
            break;
        }

        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            request->status = platform::HttpStatus::Error;
            break;
        }
    }
}

platform::HttpHandle platform::HttpGet(const char* url)
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

    // Connect
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
        request->status = HttpStatus::Error;
    }

    return MakeHttpHandle(request_index, request->generation);
}

platform::HttpHandle platform::HttpPost(const char* url, const void* body, u32 body_size, const char* content_type)
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

platform::HttpStatus platform::HttpGetStatus(const HttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request)
        return HttpStatus::None;

    return request->status;
}

int platform::HttpGetStatusCode(const HttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request)
        return 0;

    return request->status_code;
}

const u8* platform::HttpGetResponse(const HttpHandle& handle, u32* out_size)
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

void platform::HttpCancel(const HttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request)
        return;

    CleanupRequest(request);
}

void platform::HttpRelease(const HttpHandle& handle)
{
    WindowsHttpRequest* request = GetRequest(handle);
    if (!request)
        return;

    CleanupRequest(request);
    request->generation++; // Invalidate any existing handles
}

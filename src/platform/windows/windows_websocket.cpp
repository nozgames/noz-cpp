//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

constexpr int MAX_WEBSOCKETS = 8;
constexpr int MAX_MESSAGES_PER_SOCKET = 32;
constexpr u32 MAX_MESSAGE_SIZE = 1024 * 1024;  // 1MB per message
constexpr u32 RECEIVE_BUFFER_SIZE = 16 * 1024;

struct WebSocketMessage
{
    WebSocketMessageType type;
    u8* data;
    u32 size;
};

enum class WebSocketConnectState
{
    None,
    Connecting,  // Background thread is doing the blocking connect
    Connected
};

struct WindowsWebSocket
{
    HINTERNET session;
    HINTERNET connection;
    HINTERNET request;
    HINTERNET websocket;
    HANDLE thread;
    CRITICAL_SECTION cs;
    WebSocketStatus status;
    WebSocketConnectState connect_state;
    u32 generation;
    bool should_close;
    bool request_complete;
    u16 close_code;
    char close_reason[128];

    // Message queue
    WebSocketMessage messages[MAX_MESSAGES_PER_SOCKET];
    int message_read_index;
    int message_write_index;
    int message_count;

    // Receive buffer for fragmented messages
    u8* receive_buffer;
    u32 receive_size;
    u32 receive_capacity;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE receive_type;
};

struct WindowsWebSocketState
{
    WindowsWebSocket sockets[MAX_WEBSOCKETS];
    u32 next_id;
};

static WindowsWebSocketState g_ws = {};

static u32 GetSocketIndex(const PlatformWebSocketHandle& handle)
{
    return (u32)(handle.value & 0xFFFFFFFF);
}

static u32 GetSocketGeneration(const PlatformWebSocketHandle& handle)
{
    return (u32)(handle.value >> 32);
}

static PlatformWebSocketHandle MakeWebSocketHandle(u32 index, u32 generation)
{
    PlatformWebSocketHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

static WindowsWebSocket* GetSocket(const PlatformWebSocketHandle& handle)
{
    u32 index = GetSocketIndex(handle);
    u32 generation = GetSocketGeneration(handle);

    if (index >= MAX_WEBSOCKETS)
        return nullptr;

    WindowsWebSocket& ws = g_ws.sockets[index];
    if (ws.generation != generation)
        return nullptr;

    return &ws;
}

// Connect thread - does the blocking WinHTTP connect/upgrade
static DWORD WINAPI WebSocketConnectThread(LPVOID param)
{
    WindowsWebSocket* ws = (WindowsWebSocket*)param;

    // Send the upgrade request (blocking)
    if (!WinHttpSendRequest(ws->request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, nullptr, 0, 0, 0)) {
        ws->status = WebSocketStatus::Error;
        return 1;
    }

    // Receive response (blocking)
    if (!WinHttpReceiveResponse(ws->request, nullptr)) {
        ws->status = WebSocketStatus::Error;
        return 1;
    }

    // Complete the WebSocket upgrade
    ws->websocket = WinHttpWebSocketCompleteUpgrade(ws->request, 0);
    if (!ws->websocket) {
        ws->status = WebSocketStatus::Error;
        return 1;
    }

    // Close the request handle - we don't need it anymore
    WinHttpCloseHandle(ws->request);
    ws->request = nullptr;

    // Mark as connected - the main thread will start the receive thread
    ws->connect_state = WebSocketConnectState::Connected;
    ws->status = WebSocketStatus::Connected;

    return 0;
}

static void QueueMessage(WindowsWebSocket* ws, WebSocketMessageType type, const u8* data, u32 size)
{
    EnterCriticalSection(&ws->cs);

    if (ws->message_count < MAX_MESSAGES_PER_SOCKET)
    {
        WebSocketMessage& msg = ws->messages[ws->message_write_index];
        msg.type = type;
        msg.data = (u8*)Alloc(ALLOCATOR_DEFAULT, size + 1);  // +1 for null terminator
        memcpy(msg.data, data, size);
        msg.data[size] = '\0';  // Null terminate for convenience
        msg.size = size;

        ws->message_write_index = (ws->message_write_index + 1) % MAX_MESSAGES_PER_SOCKET;
        ws->message_count++;
    }

    LeaveCriticalSection(&ws->cs);
}

static void AppendToReceiveBuffer(WindowsWebSocket* ws, const u8* data, u32 size)
{
    u32 new_size = ws->receive_size + size;
    if (new_size > ws->receive_capacity)
    {
        u32 new_capacity = Max(ws->receive_capacity * 2, new_size + RECEIVE_BUFFER_SIZE);
        new_capacity = Min(new_capacity, MAX_MESSAGE_SIZE);

        if (new_size > new_capacity)
        {
            LogWarning("WebSocket message too large, truncating");
            size = new_capacity - ws->receive_size;
            new_size = new_capacity;
        }

        u8* new_buffer = (u8*)Alloc(ALLOCATOR_DEFAULT, new_capacity);
        if (ws->receive_buffer)
        {
            memcpy(new_buffer, ws->receive_buffer, ws->receive_size);
            Free(ws->receive_buffer);
        }
        ws->receive_buffer = new_buffer;
        ws->receive_capacity = new_capacity;
    }

    memcpy(ws->receive_buffer + ws->receive_size, data, size);
    ws->receive_size = new_size;
}

static DWORD WINAPI WebSocketReceiveThread(LPVOID param)
{
    WindowsWebSocket* ws = (WindowsWebSocket*)param;
    u8 buffer[RECEIVE_BUFFER_SIZE];

    while (!ws->should_close && ws->status == WebSocketStatus::Connected)
    {
        DWORD bytes_read = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE buffer_type = WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;

        DWORD error = WinHttpWebSocketReceive(
            ws->websocket,
            buffer,
            sizeof(buffer),
            &bytes_read,
            &buffer_type);

        if (error != ERROR_SUCCESS)
        {
            if (!ws->should_close)
            {
                EnterCriticalSection(&ws->cs);
                ws->status = WebSocketStatus::Error;
                LeaveCriticalSection(&ws->cs);
            }
            break;
        }

        switch (buffer_type)
        {
            case WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:
            case WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:
            {
                // Complete message (or final fragment)
                if (ws->receive_size > 0)
                {
                    // This is the final fragment - use the fragment type we've been accumulating
                    WebSocketMessageType msg_type =
                        (ws->receive_type == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE)
                        ? WebSocketMessageType::Text
                        : WebSocketMessageType::Binary;

                    AppendToReceiveBuffer(ws, buffer, bytes_read);
                    QueueMessage(ws, msg_type, ws->receive_buffer, ws->receive_size);
                    ws->receive_size = 0;
                }
                else
                {
                    // Single complete message, no fragments
                    WebSocketMessageType msg_type =
                        (buffer_type == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)
                        ? WebSocketMessageType::Text
                        : WebSocketMessageType::Binary;

                    QueueMessage(ws, msg_type, buffer, bytes_read);
                }
                break;
            }

            case WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:
            case WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:
            {
                // Fragment - accumulate
                if (ws->receive_size == 0)
                {
                    ws->receive_type = buffer_type;
                }
                AppendToReceiveBuffer(ws, buffer, bytes_read);
                break;
            }

            case WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE:
            {
                EnterCriticalSection(&ws->cs);
                ws->status = WebSocketStatus::Closed;

                // Get close status
                USHORT close_code = 0;
                DWORD reason_length = 0;
                WinHttpWebSocketQueryCloseStatus(
                    ws->websocket,
                    &close_code,
                    ws->close_reason,
                    sizeof(ws->close_reason) - 1,
                    &reason_length);
                ws->close_code = close_code;
                ws->close_reason[reason_length] = '\0';

                LeaveCriticalSection(&ws->cs);
                return 0;
            }

            default:
                break;
        }
    }

    return 0;
}

static void CleanupSocket(WindowsWebSocket* ws)
{
    ws->should_close = true;

    // Close websocket handle FIRST to unblock the receive thread
    // WinHttpWebSocketReceive will return an error once the handle is closed
    if (ws->websocket)
    {
        WinHttpWebSocketClose(ws->websocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
        WinHttpCloseHandle(ws->websocket);
        ws->websocket = nullptr;
    }

    // Now wait for the thread to exit (should be quick since receive was unblocked)
    if (ws->thread)
    {
        WaitForSingleObject(ws->thread, 1000);
        CloseHandle(ws->thread);
        ws->thread = nullptr;
    }

    if (ws->request)
    {
        WinHttpCloseHandle(ws->request);
        ws->request = nullptr;
    }

    if (ws->connection)
    {
        WinHttpCloseHandle(ws->connection);
        ws->connection = nullptr;
    }

    if (ws->session)
    {
        WinHttpCloseHandle(ws->session);
        ws->session = nullptr;
    }

    // Free messages
    for (int i = 0; i < MAX_MESSAGES_PER_SOCKET; i++)
    {
        if (ws->messages[i].data)
        {
            Free(ws->messages[i].data);
            ws->messages[i].data = nullptr;
        }
    }

    if (ws->receive_buffer)
    {
        Free(ws->receive_buffer);
        ws->receive_buffer = nullptr;
    }

    DeleteCriticalSection(&ws->cs);
}

void PlatformInitWebSocket()
{
    memset(&g_ws, 0, sizeof(g_ws));
}

void PlatformShutdownWebSocket()
{
    for (int i = 0; i < MAX_WEBSOCKETS; i++)
    {
        WindowsWebSocket& ws = g_ws.sockets[i];
        if (ws.status != WebSocketStatus::None)
        {
            CleanupSocket(&ws);
        }
    }
    memset(&g_ws, 0, sizeof(g_ws));
}

void PlatformUpdateWebSocket()
{
    for (int i = 0; i < MAX_WEBSOCKETS; i++) {
        WindowsWebSocket& ws = g_ws.sockets[i];

        // Check if connect thread finished and we need to start receive thread
        if (ws.status == WebSocketStatus::Connected &&
            ws.connect_state == WebSocketConnectState::Connected &&
            ws.thread == nullptr) {

            ws.thread = CreateThread(nullptr, 0, WebSocketReceiveThread, &ws, 0, nullptr);
            if (!ws.thread) {
                ws.status = WebSocketStatus::Error;
            }
        }
    }
}

PlatformWebSocketHandle PlatformConnectWebSocket(const char* url) {
    WindowsWebSocket* ws = nullptr;
    u32 socket_index = 0;
    for (int i = 0; i < MAX_WEBSOCKETS; i++) {
        if (g_ws.sockets[i].status == WebSocketStatus::None ||
            g_ws.sockets[i].status == WebSocketStatus::Closed ||
            g_ws.sockets[i].status == WebSocketStatus::Error) {
            if (g_ws.sockets[i].status != WebSocketStatus::None)
                CleanupSocket(&g_ws.sockets[i]);

            ws = &g_ws.sockets[i];
            socket_index = i;
            break;
        }
    }

    if (!ws) {
        return MakeWebSocketHandle(0, 0xFFFFFFFF);
    }

    // Initialize
    memset(ws, 0, sizeof(WindowsWebSocket));
    InitializeCriticalSection(&ws->cs);
    ws->status = WebSocketStatus::Connecting;
    ws->connect_state = WebSocketConnectState::None;
    ws->generation = ++g_ws.next_id;

    // Parse URL
    URL_COMPONENTS url_components = {};
    url_components.dwStructSize = sizeof(url_components);
    url_components.dwSchemeLength = (DWORD)-1;
    url_components.dwHostNameLength = (DWORD)-1;
    url_components.dwUrlPathLength = (DWORD)-1;
    url_components.dwExtraInfoLength = (DWORD)-1;

    // WinHttpCrackUrl doesn't recognize wss:// scheme, so replace with https:// for parsing
    bool is_wss = strncmp(url, "wss://", 6) == 0;
    bool is_ws = strncmp(url, "ws://", 5) == 0;

    const char* parse_url = url;
    char temp_url[2048];
    if (is_wss) {
        snprintf(temp_url, sizeof(temp_url), "https://%s", url + 6);
        parse_url = temp_url;
    } else if (is_ws) {
        snprintf(temp_url, sizeof(temp_url), "http://%s", url + 5);
        parse_url = temp_url;
    }

    int url_len = (int)strlen(parse_url);
    int wide_len = MultiByteToWideChar(CP_UTF8, 0, parse_url, url_len, nullptr, 0);
    wchar_t* wide_url = (wchar_t*)Alloc(ALLOCATOR_SCRATCH, (wide_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, parse_url, url_len, wide_url, wide_len);
    wide_url[wide_len] = 0;

    if (!WinHttpCrackUrl(wide_url, 0, 0, &url_components)) {
        ws->status = WebSocketStatus::Error;
        return MakeWebSocketHandle(socket_index, ws->generation);
    }

    // Extract hostname
    wchar_t hostname[256] = {};
    wcsncpy_s(hostname, url_components.lpszHostName, url_components.dwHostNameLength);

    // Determine if secure (wss://)
    bool secure = (url_components.nScheme == INTERNET_SCHEME_HTTPS) ||
                  (wcsncmp(wide_url, L"wss://", 6) == 0);

    // Create synchronous session
    ws->session = WinHttpOpen(
        L"NoZ/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        nullptr,
        nullptr,
        0);  // No WINHTTP_FLAG_ASYNC - synchronous mode

    if (!ws->session) {
        ws->status = WebSocketStatus::Error;
        return MakeWebSocketHandle(socket_index, ws->generation);
    }

    // Connect
    INTERNET_PORT port = url_components.nPort;
    if (port == 0)
        port = secure ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    ws->connection = WinHttpConnect(
        ws->session,
        hostname,
        port,
        0);
    if (!ws->connection) {
        ws->status = WebSocketStatus::Error;
        return MakeWebSocketHandle(socket_index, ws->generation);
    }

    // Build path with query string
    wchar_t path[2048] = L"/";
    if (url_components.dwUrlPathLength > 0) {
        wcsncpy_s(path, url_components.lpszUrlPath, url_components.dwUrlPathLength);
    }
    if (url_components.dwExtraInfoLength > 0) {
        wcsncat_s(path, url_components.lpszExtraInfo, url_components.dwExtraInfoLength);
    }

    // Create request
    DWORD flags = secure ? WINHTTP_FLAG_SECURE : 0;
    ws->request = WinHttpOpenRequest(
        ws->connection,
        L"GET",
        path,
        nullptr,
        nullptr,
        nullptr,
        flags);

    if (!ws->request) {
        ws->status = WebSocketStatus::Error;
        return MakeWebSocketHandle(socket_index, ws->generation);
    }

    if (!WinHttpSetOption(ws->request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0)) {
        ws->status = WebSocketStatus::Error;
        return MakeWebSocketHandle(socket_index, ws->generation);
    }

    // Start connect thread to do the blocking send/receive/upgrade
    ws->connect_state = WebSocketConnectState::Connecting;

    HANDLE connect_thread = CreateThread(nullptr, 0, WebSocketConnectThread, ws, 0, nullptr);
    if (!connect_thread) {
        ws->status = WebSocketStatus::Error;
        return MakeWebSocketHandle(socket_index, ws->generation);
    }

    // We don't need to keep the connect thread handle - it will set status when done
    CloseHandle(connect_thread);

    return MakeWebSocketHandle(socket_index, ws->generation);
}

void PlatformSend(const PlatformWebSocketHandle& handle, const char* text) {
    WindowsWebSocket* ws = GetSocket(handle);
    if (!ws || ws->status != WebSocketStatus::Connected)
        return;

    u32 len = (u32)strlen(text);
    WinHttpWebSocketSend(
        ws->websocket,
        WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        (PVOID)text,
        len);
}

void PlatformSendBinary(const PlatformWebSocketHandle& handle, const void* data, u32 size) {
    WindowsWebSocket* ws = GetSocket(handle);
    if (!ws || ws->status != WebSocketStatus::Connected)
        return;

    WinHttpWebSocketSend(
        ws->websocket,
        WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
        (PVOID)data,
        size);
}

void PlatformClose(const PlatformWebSocketHandle& handle, u16 code, const char* reason) {
    WindowsWebSocket* ws = GetSocket(handle);
    if (!ws)
        return;

    EnterCriticalSection(&ws->cs);

    if (ws->status == WebSocketStatus::Connected) {
        ws->status = WebSocketStatus::Closing;
        ws->should_close = true;

        DWORD reason_len = reason ? (DWORD)strlen(reason) : 0;
        WinHttpWebSocketClose(
            ws->websocket,
            code,
            (PVOID)reason,
            reason_len);
    }

    LeaveCriticalSection(&ws->cs);
}

void PlatformFree(const PlatformWebSocketHandle& handle) {
    WindowsWebSocket* ws = GetSocket(handle);
    if (!ws)
        return;

    CleanupSocket(ws);
    ws->status = WebSocketStatus::None;
    ws->generation++;
}

WebSocketStatus PlatformGetStatus(const PlatformWebSocketHandle& handle)
{
    WindowsWebSocket* ws = GetSocket(handle);
    if (!ws)
        return WebSocketStatus::None;

    EnterCriticalSection(&ws->cs);
    WebSocketStatus status = ws->status;
    LeaveCriticalSection(&ws->cs);

    return status;
}

bool PlatformHasMessages(const PlatformWebSocketHandle& handle)
{
    WindowsWebSocket* ws = GetSocket(handle);
    if (!ws)
        return false;

    EnterCriticalSection(&ws->cs);
    bool has_message = ws->message_count > 0;
    LeaveCriticalSection(&ws->cs);

    return has_message;
}

bool PlatformGetMessage(const PlatformWebSocketHandle& handle, WebSocketMessageType* out_type, u8** out_data, u32* out_size) {
    WindowsWebSocket* ws = GetSocket(handle);
    if (!ws)
        return false;

    EnterCriticalSection(&ws->cs);

    if (ws->message_count == 0) {
        LeaveCriticalSection(&ws->cs);
        return false;
    }

    WebSocketMessage& msg = ws->messages[ws->message_read_index];
    if (out_type) *out_type = msg.type;
    if (out_data) *out_data = msg.data;
    if (out_size) *out_size = msg.size;

    LeaveCriticalSection(&ws->cs);
    return true;
}

void PlatformPopMessage(const PlatformWebSocketHandle& handle)
{
    WindowsWebSocket* ws = GetSocket(handle);
    if (!ws)
        return;

    EnterCriticalSection(&ws->cs);

    if (ws->message_count > 0)
    {
        WebSocketMessage& msg = ws->messages[ws->message_read_index];
        if (msg.data)
        {
            Free(msg.data);
            msg.data = nullptr;
        }
        msg.size = 0;

        ws->message_read_index = (ws->message_read_index + 1) % MAX_MESSAGES_PER_SOCKET;
        ws->message_count--;
    }

    LeaveCriticalSection(&ws->cs);
}

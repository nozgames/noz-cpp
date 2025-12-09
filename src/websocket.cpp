//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "pch.h"
#include "platform.h"

constexpr int MAX_WEBSOCKETS = 8;

struct WebSocketImpl {
    PlatformWebSocketHandle handle;
    WebSocketMessageCallback on_message;
    WebSocketCloseCallback on_close;
    WebSocketConnectCallback on_connect;
    WebSocketStatus last_status;
    bool active;
    bool connect_callback_fired;
};

static WebSocketImpl g_websockets[MAX_WEBSOCKETS] = {};

static WebSocketImpl* AllocWebSocket() {
    for (int i = 0; i < MAX_WEBSOCKETS; i++) {
        if (!g_websockets[i].active) {
            g_websockets[i] = {};
            g_websockets[i].active = true;
            return &g_websockets[i];
        }
    }
    LogWarning("No free WebSocket slots");
    return nullptr;
}

WebSocket* WebSocketConnect(const char* url) {
    WebSocketImpl* ws = AllocWebSocket();
    if (!ws)
        return nullptr;

    ws->handle = PlatformConnectWebSocket(url);
    ws->last_status = WebSocketStatus::Connecting;
    ws->connect_callback_fired = false;

    return (WebSocket*)ws;
}

void WebSocketOnConnect(WebSocket* socket, WebSocketConnectCallback callback)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    ws->on_connect = callback;
}

void WebSocketOnMessage(WebSocket* socket, WebSocketMessageCallback callback)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    ws->on_message = callback;
}

void WebSocketOnClose(WebSocket* socket, WebSocketCloseCallback callback)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    ws->on_close = callback;
}

WebSocketStatus WebSocketGetStatus(WebSocket* socket) {
    if (!socket)
        return WebSocketStatus::None;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    return PlatformGetStatus(ws->handle);
}

bool WebSocketIsConnected(WebSocket* socket)
{
    return WebSocketGetStatus(socket) == WebSocketStatus::Connected;
}

bool WebSocketIsConnecting(WebSocket* socket)
{
    return WebSocketGetStatus(socket) == WebSocketStatus::Connecting;
}

void WebSocketSend(WebSocket* socket, const char* text)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    PlatformSend(ws->handle, text);
}

void WebSocketSendBinary(WebSocket* socket, const void* data, u32 size)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    PlatformSendBinary(ws->handle, data, size);
}

bool WebSocketHasMessage(WebSocket* socket)
{
    if (!socket)
        return false;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    return PlatformHasMessages(ws->handle);
}

bool WebSocketPeekMessage(WebSocket* socket, WebSocketMessageType* out_type, const u8** out_data, u32* out_size)
{
    if (!socket)
        return false;

    WebSocketImpl* ws = (WebSocketImpl*)socket;

    WebSocketMessageType ptype;
    bool result = PlatformGetMessage(ws->handle, &ptype, out_data, out_size);

    if (result && out_type)
        *out_type = ptype;

    return result;
}

const char* WebSocketPeekMessageText(WebSocket* socket)
{
    const u8* data = nullptr;
    u32 size = 0;
    WebSocketMessageType type;

    if (!WebSocketPeekMessage(socket, &type, &data, &size))
        return nullptr;

    if (type != WebSocketMessageType::Text)
        return nullptr;

    // Data is already null-terminated by the platform layer
    return (const char*)data;
}

void WebSocketPopMessage(WebSocket* socket)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    PlatformPopMessage(ws->handle);
}

void WebSocketClose(WebSocket* socket, u16 code, const char* reason)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    PlatformClose(ws->handle, code, reason);
}

void WebSocketRelease(WebSocket* socket)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;

    PlatformFree(ws->handle);

    ws->on_message = nullptr;
    ws->on_close = nullptr;
    ws->on_connect = nullptr;
    ws->active = false;
}

void InitWebSocket()
{
    for (int i = 0; i < MAX_WEBSOCKETS; i++)
        g_websockets[i] = {};

    PlatformInitWebSocket();
}

void ShutdownWebSocket()
{
    for (int i = 0; i < MAX_WEBSOCKETS; i++)
    {
        g_websockets[i] = {};
    }

    PlatformShutdownWebSocket();
}

void UpdateWebSocket()
{
    PlatfrormUpdateWebSocket();

    for (int i = 0; i < MAX_WEBSOCKETS; i++)
    {
        WebSocketImpl& ws = g_websockets[i];
        if (!ws.active)
            continue;

        WebSocketStatus current = PlatformGetStatus(ws.handle);

        // Check for connection success/failure
        if (!ws.connect_callback_fired)
        {
            if (current == WebSocketStatus::Connected)
            {
                ws.connect_callback_fired = true;
                if (ws.on_connect)
                    ws.on_connect((WebSocket*)&ws, true);
            }
            else if (current == WebSocketStatus::Error)
            {
                ws.connect_callback_fired = true;
                if (ws.on_connect)
                    ws.on_connect((WebSocket*)&ws, false);
            }
        }

        // Dispatch messages
        if (ws.on_message) {
            while (PlatformHasMessages(ws.handle)) {
                WebSocketMessageType ptype;
                const u8* data;
                u32 size;

                if (PlatformGetMessage(ws.handle, &ptype, &data, &size)) {
                    ws.on_message((WebSocket*)&ws, ptype, data, size);
                    PlatformPopMessage(ws.handle);
                } else {
                    break;
                }
            }
        }

        // Check for close
        if (ws.last_status == WebSocketStatus::Connected &&
            (current == WebSocketStatus::Closed || current == WebSocketStatus::Error)) {
            if (ws.on_close) {
                // TODO: Get actual close code/reason from platform
                u16 code = (current == WebSocketStatus::Error) ? 1006 : 1000;
                ws.on_close((WebSocket*)&ws, code, nullptr);
            }
        }

        ws.last_status = current;
    }
}

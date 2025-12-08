//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "pch.h"
#include "platform.h"

constexpr int MAX_WEBSOCKETS = 8;

struct WebSocketImpl
{
    platform::WebSocketHandle handle;
    WebSocketMessageCallback on_message;
    WebSocketCloseCallback on_close;
    WebSocketConnectCallback on_connect;
    WebSocketStatus last_status;
    bool active;
    bool connect_callback_fired;
};

static WebSocketImpl g_websockets[MAX_WEBSOCKETS] = {};

static WebSocketImpl* AllocWebSocket()
{
    for (int i = 0; i < MAX_WEBSOCKETS; i++)
    {
        if (!g_websockets[i].active)
        {
            g_websockets[i] = {};
            g_websockets[i].active = true;
            return &g_websockets[i];
        }
    }
    LogWarning("No free WebSocket slots");
    return nullptr;
}

static WebSocketStatus ConvertStatus(platform::WebSocketStatus status)
{
    switch (status)
    {
        case platform::WebSocketStatus::None:       return WebSocketStatus::None;
        case platform::WebSocketStatus::Connecting: return WebSocketStatus::Connecting;
        case platform::WebSocketStatus::Connected:  return WebSocketStatus::Connected;
        case platform::WebSocketStatus::Closing:    return WebSocketStatus::Closing;
        case platform::WebSocketStatus::Closed:     return WebSocketStatus::Closed;
        case platform::WebSocketStatus::Error:      return WebSocketStatus::Error;
    }
    return WebSocketStatus::None;
}

static WebSocketMessageType ConvertMessageType(platform::WebSocketMessageType type)
{
    switch (type)
    {
        case platform::WebSocketMessageType::Text:   return WebSocketMessageType::Text;
        case platform::WebSocketMessageType::Binary: return WebSocketMessageType::Binary;
    }
    return WebSocketMessageType::Text;
}

WebSocket* WebSocketConnect(const char* url)
{
    WebSocketImpl* ws = AllocWebSocket();
    if (!ws)
        return nullptr;

    ws->handle = platform::WebSocketConnect(url);
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

WebSocketStatus WebSocketGetStatus(WebSocket* socket)
{
    if (!socket)
        return WebSocketStatus::None;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    return ConvertStatus(platform::WebSocketGetStatus(ws->handle));
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
    platform::WebSocketSend(ws->handle, text);
}

void WebSocketSendBinary(WebSocket* socket, const void* data, u32 size)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    platform::WebSocketSendBinary(ws->handle, data, size);
}

bool WebSocketHasMessage(WebSocket* socket)
{
    if (!socket)
        return false;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    return platform::WebSocketHasMessage(ws->handle);
}

bool WebSocketPeekMessage(WebSocket* socket, WebSocketMessageType* out_type, const u8** out_data, u32* out_size)
{
    if (!socket)
        return false;

    WebSocketImpl* ws = (WebSocketImpl*)socket;

    platform::WebSocketMessageType ptype;
    bool result = platform::WebSocketGetMessage(ws->handle, &ptype, out_data, out_size);

    if (result && out_type)
        *out_type = ConvertMessageType(ptype);

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
    platform::WebSocketPopMessage(ws->handle);
}

void WebSocketClose(WebSocket* socket, u16 code, const char* reason)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    platform::WebSocketClose(ws->handle, code, reason);
}

void WebSocketRelease(WebSocket* socket)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;

    platform::WebSocketRelease(ws->handle);

    ws->on_message = nullptr;
    ws->on_close = nullptr;
    ws->on_connect = nullptr;
    ws->active = false;
}

void InitWebSocket()
{
    for (int i = 0; i < MAX_WEBSOCKETS; i++)
        g_websockets[i] = {};

    platform::InitializeWebSocket();
}

void ShutdownWebSocket()
{
    for (int i = 0; i < MAX_WEBSOCKETS; i++)
    {
        g_websockets[i] = {};
    }

    platform::ShutdownWebSocket();
}

void UpdateWebSocket()
{
    platform::UpdateWebSocket();

    for (int i = 0; i < MAX_WEBSOCKETS; i++)
    {
        WebSocketImpl& ws = g_websockets[i];
        if (!ws.active)
            continue;

        WebSocketStatus current = ConvertStatus(platform::WebSocketGetStatus(ws.handle));

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
        if (ws.on_message)
        {
            while (platform::WebSocketHasMessage(ws.handle))
            {
                platform::WebSocketMessageType ptype;
                const u8* data;
                u32 size;

                if (platform::WebSocketGetMessage(ws.handle, &ptype, &data, &size))
                {
                    ws.on_message((WebSocket*)&ws, ConvertMessageType(ptype), data, size);
                    platform::WebSocketPopMessage(ws.handle);
                }
                else
                {
                    break;
                }
            }
        }

        // Check for close
        if (ws.last_status == WebSocketStatus::Connected &&
            (current == WebSocketStatus::Closed || current == WebSocketStatus::Error))
        {
            if (ws.on_close)
            {
                // TODO: Get actual close code/reason from platform
                u16 code = (current == WebSocketStatus::Error) ? 1006 : 1000;
                ws.on_close((WebSocket*)&ws, code, nullptr);
            }
        }

        ws.last_status = current;
    }
}

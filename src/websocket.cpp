//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "pch.h"
#include "platform.h"

#if defined(NOZ_WEBSOCKET)
#include <zlib.h>
#endif

constexpr int MAX_WEBSOCKETS = 32;

struct WebSocketImpl {
    PlatformWebSocketHandle handle;
    WebSocketProc proc;
    void* user_data;
    WebSocketStatus last_status;
    bool active;
    bool connect_callback_fired;
};

static WebSocketImpl g_websockets[MAX_WEBSOCKETS] = {};

static u8* DecompressGzip(u8 *compressed_data, u32 compressed_size, u32 *out_decompressed_size) {
#if defined(NOZ_WEBSOCKET)
    // Check for gzip magic number
    if (compressed_size < 2 || compressed_data[0] != 0x1f || compressed_data[1] != 0x8b) {
        // Not gzip, return null
        return nullptr;
    }

    // Start with a reasonable buffer size (small gzip messages decompress to much larger)
    u32 buffer_size = 1024 * 1024;  // 1MB initial buffer
    u8* decompressed = (u8*)Alloc(ALLOCATOR_DEFAULT, buffer_size);

    z_stream stream = {};
    stream.next_in = (Bytef*)compressed_data;
    stream.avail_in = compressed_size;
    stream.next_out = decompressed;
    stream.avail_out = buffer_size;

    // Initialize with gzip decoding (windowBits = 15 + 16 for gzip)
    if (inflateInit2(&stream, 15 + 16) != Z_OK) {
        Free(decompressed);
        return nullptr;
    }

    int ret = inflate(&stream, Z_FINISH);

    // If buffer was too small, grow and retry
    while (ret == Z_BUF_ERROR) {
        u32 new_buffer_size = buffer_size * 2;
        u8* new_buffer = (u8*)Alloc(ALLOCATOR_DEFAULT, new_buffer_size);
        memcpy(new_buffer, decompressed, stream.total_out);
        Free(decompressed);
        decompressed = new_buffer;

        stream.next_out = decompressed + stream.total_out;
        stream.avail_out = new_buffer_size - stream.total_out;
        buffer_size = new_buffer_size;

        ret = inflate(&stream, Z_FINISH);
    }

    if (ret != Z_STREAM_END) {
        inflateEnd(&stream);
        Free(decompressed);
        LogWarning("Gzip decompression failed: %d", ret);
        return nullptr;
    }

    *out_decompressed_size = stream.total_out;
    inflateEnd(&stream);
    return decompressed;
#else
    *out_decompressed_size = compressed_size;
    return compressed_data;
#endif
}

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

WebSocket* CreateWebSocket(Allocator* allocator, const char* url, WebSocketProc proc, void* user_data) {
    (void)allocator;
    WebSocketImpl* ws = AllocWebSocket();
    if (!ws)
        return nullptr;

    ws->handle = PlatformConnectWebSocket(url);
    ws->last_status = WebSocketStatus::Connecting;
    ws->connect_callback_fired = false;
    ws->proc = proc;
    ws->user_data = user_data;

    return reinterpret_cast<WebSocket *>(ws);
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

bool WebSocketPeekMessage(WebSocket* socket, WebSocketMessageType* out_type, u8** out_data, u32* out_size) {
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
    u8* data = nullptr;
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

void Close(WebSocket* socket, u16 code, const char* reason)
{
    if (!socket)
        return;

    WebSocketImpl* ws = (WebSocketImpl*)socket;
    PlatformClose(ws->handle, code, reason);
}

void Free(WebSocket* socket) {
    if (!socket) return;

    WebSocketImpl* impl = reinterpret_cast<WebSocketImpl *>(socket);
    PlatformFree(impl->handle);

    impl->proc = nullptr;
    impl->user_data = nullptr;
    impl->active = false;
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
    PlatformUpdateWebSocket();

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
                ws.proc(reinterpret_cast<WebSocket *>(&ws), WEBSOCKET_EVENT_CONNECTED, nullptr, 0, ws.user_data);
            }
            else if (current == WebSocketStatus::Error)
            {
                ws.connect_callback_fired = true;
                ws.proc(reinterpret_cast<WebSocket *>(&ws), WEBSOCKET_EVENT_CLOSED, nullptr, 0, ws.user_data);
            }
        }

        // Dispatch messages
        while (PlatformHasMessages(ws.handle)) {
            WebSocketMessageType ptype;
            u8* data;
            u32 size;

            if (PlatformGetMessage(ws.handle, &ptype, &data, &size)) {
                // Check if message is gzip-compressed and decompress if needed
                u32 decompressed_size = 0;
                u8* decompressed_data = DecompressGzip(data, size, &decompressed_size);

                const u8* final_data = decompressed_data ? decompressed_data : data;
                u32 final_size = decompressed_data ? decompressed_size : size;

                WebSocketEventMessage event_data = {};
                event_data.data = final_data;
                event_data.size = final_size;
                ws.proc(
                    reinterpret_cast<WebSocket *>(&ws),
                    WEBSOCKET_EVENT_MESSAGE,
                    &event_data,
                    sizeof(WebSocketEventMessage),
                    ws.user_data);

                // Free decompressed buffer if we allocated one
                if (decompressed_data) {
                    Free(decompressed_data);
                }

                PlatformPopMessage(ws.handle);
            } else {
                break;
            }
        }

        // Check for close
        if (ws.last_status == WebSocketStatus::Connected &&
            (current == WebSocketStatus::Closed || current == WebSocketStatus::Error)) {
            // if (ws.on_close) {
            //     // TODO: Get actual close code/reason from platform
            //     u16 code = (current == WebSocketStatus::Error) ? 1006 : 1000;
            //     ws.on_close((WebSocket*)&ws, code, nullptr);
            // }
        }

        ws.last_status = current;
    }
}

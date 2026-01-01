//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if defined(NOZ_WEBSOCKET)
#include <zlib.h>
#endif

namespace noz {

struct WebSocket {
    PlatformWebSocketHandle handle;
    WebSocketProc proc;
    void* user_data;
    WebSocketStatus last_status;
    WebSocketFlags flags;
    bool connect_callback_fired;
};

static void WebSocketDestructor(void* ptr) {
    WebSocket* ws = (WebSocket*)ptr;
    PlatformFree(ws->handle);
}

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
    (void)compressed_data;
    (void)compressed_size;
    *out_decompressed_size = 0;
    return nullptr;
#endif
}

WebSocket* CreateWebSocket(const WebSocketConfig& config) {
    WebSocket* ws = (WebSocket*)Alloc(ALLOCATOR_DEFAULT, sizeof(WebSocket), WebSocketDestructor);
    if (!ws) {
        return nullptr;
    }

    ws->handle = PlatformConnectWebSocket(config.url.value);
    ws->last_status = WebSocketStatus::Connecting;
    ws->connect_callback_fired = false;
    ws->proc = config.on_event;
    ws->user_data = config.user_data;
    ws->flags = config.flags;

    return ws;
}

void Update(WebSocket* ws) {
    if (!ws) return;

    // Update platform state (starts receive thread after connect)
    PlatformUpdateWebSocket();

    WebSocketStatus current = PlatformGetStatus(ws->handle);

    // Check for connection success/failure
    if (!ws->connect_callback_fired) {
        if (current == WebSocketStatus::Connected) {
            ws->connect_callback_fired = true;
            if (ws->proc) {
                ws->proc(ws, WEBSOCKET_EVENT_CONNECTED, nullptr, 0, ws->user_data);
            }
        }
        else if (current == WebSocketStatus::Error) {
            ws->connect_callback_fired = true;
            if (ws->proc) {
                ws->proc(ws, WEBSOCKET_EVENT_CLOSED, nullptr, 0, ws->user_data);
            }
        }
    }

    // Check for disconnect
    if (current == WebSocketStatus::Error || current == WebSocketStatus::Closed) {
        if (ws->last_status == WebSocketStatus::Connected) {
            if (ws->proc) {
                ws->proc(ws, WEBSOCKET_EVENT_CLOSED, nullptr, 0, ws->user_data);
            }
        }
    }
    ws->last_status = current;

    // Dispatch messages
    if (current == WebSocketStatus::Connected) {
        while (PlatformHasMessages(ws->handle)) {
            WebSocketMessageType ptype;
            u8* data;
            u32 size;

            if (PlatformGetMessage(ws->handle, &ptype, &data, &size)) {
                const u8* final_data = data;
                u32 final_size = size;
                u8* decompressed_data = nullptr;

                // Check if gzip decompression is enabled and message is gzip-compressed
                if (ws->flags & WEBSOCKET_FLAG_GZIP) {
                    u32 decompressed_size = 0;
                    decompressed_data = DecompressGzip(data, size, &decompressed_size);
                    if (decompressed_data) {
                        final_data = decompressed_data;
                        final_size = decompressed_size;
                    }
                }

                if (ws->proc) {
                    WebSocketEventMessage event_data = {};
                    event_data.data = final_data;
                    event_data.size = final_size;
                    ws->proc(ws, WEBSOCKET_EVENT_MESSAGE, &event_data, sizeof(WebSocketEventMessage), ws->user_data);
                }

                Free(decompressed_data);
                PlatformPopMessage(ws->handle);
            } else {
                break;
            }
        }
    }
}

WebSocketStatus GetStatus(WebSocket* ws) {
    if (!ws) return WebSocketStatus::None;
    return PlatformGetStatus(ws->handle);
}

bool IsConnected(WebSocket* ws) {
    return GetStatus(ws) == WebSocketStatus::Connected;
}

bool IsConnecting(WebSocket* ws) {
    return GetStatus(ws) == WebSocketStatus::Connecting;
}

void Send(WebSocket* ws, const char* text) {
    if (!ws) return;
    PlatformSend(ws->handle, text);
}

void SendBinary(WebSocket* ws, const void* data, u32 size) {
    if (!ws) return;
    PlatformSendBinary(ws->handle, data, size);
}

bool HasMessage(WebSocket* ws) {
    if (!ws) return false;
    return PlatformHasMessages(ws->handle);
}

bool PeekMessage(WebSocket* ws, WebSocketMessageType* out_type, u8** out_data, u32* out_size) {
    if (!ws) return false;

    WebSocketMessageType ptype;
    bool result = PlatformGetMessage(ws->handle, &ptype, out_data, out_size);

    if (result && out_type)
        *out_type = ptype;

    return result;
}

const char* PeekMessageText(WebSocket* ws) {
    u8* data = nullptr;
    u32 size = 0;
    WebSocketMessageType type;

    if (!PeekMessage(ws, &type, &data, &size))
        return nullptr;

    if (type != WebSocketMessageType::Text)
        return nullptr;

    return (const char*)data;
}

void PopMessage(WebSocket* ws) {
    if (!ws) return;
    PlatformPopMessage(ws->handle);
}

void Close(WebSocket* ws, u16 code, const char* reason) {
    if (!ws) return;
    PlatformClose(ws->handle, code, reason);
}

}

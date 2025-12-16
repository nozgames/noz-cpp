//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Web/Emscripten WebSocket implementation
//

#include "../../platform.h"
#include "../../internal.h"

#include <emscripten.h>
#include <emscripten/websocket.h>

constexpr int MAX_WEBSOCKETS = 32;
constexpr int MAX_MESSAGES_PER_SOCKET = 32;
constexpr u32 MAX_MESSAGE_SIZE = 64 * 1024;

struct WebSocketMessage {
    WebSocketMessageType type;
    u8* data;
    u32 size;
};

struct WebWebSocket {
    EMSCRIPTEN_WEBSOCKET_T handle;
    WebSocketStatus status;
    u32 generation;
    u16 close_code;

    // Message queue
    WebSocketMessage messages[MAX_MESSAGES_PER_SOCKET];
    int message_read_index;
    int message_write_index;
    int message_count;
};

struct WebSocketState {
    WebWebSocket sockets[MAX_WEBSOCKETS];
    u32 next_id;
};

static WebSocketState g_ws = {};

static u32 GetSocketIndex(const PlatformWebSocketHandle& handle) {
    return (u32)(handle.value & 0xFFFFFFFF);
}

static u32 GetSocketGeneration(const PlatformWebSocketHandle& handle) {
    return (u32)(handle.value >> 32);
}

static PlatformWebSocketHandle MakeWebSocketHandle(u32 index, u32 generation) {
    PlatformWebSocketHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

static WebWebSocket* GetSocket(const PlatformWebSocketHandle& handle) {
    u32 index = GetSocketIndex(handle);
    u32 generation = GetSocketGeneration(handle);

    if (index >= MAX_WEBSOCKETS)
        return nullptr;

    WebWebSocket& ws = g_ws.sockets[index];
    if (ws.generation != generation)
        return nullptr;

    return &ws;
}

static WebWebSocket* GetSocketByEmscriptenHandle(EMSCRIPTEN_WEBSOCKET_T em_handle) {
    for (int i = 0; i < MAX_WEBSOCKETS; i++) {
        if (g_ws.sockets[i].handle == em_handle && g_ws.sockets[i].status != WebSocketStatus::Closed) {
            return &g_ws.sockets[i];
        }
    }
    return nullptr;
}

static void QueueMessage(WebWebSocket* ws, WebSocketMessageType type, const u8* data, u32 size) {
    if (ws->message_count >= MAX_MESSAGES_PER_SOCKET) {
        LogError("WebSocket: Message queue full, dropping message");
        return;
    }

    WebSocketMessage& msg = ws->messages[ws->message_write_index];

    // Allocate and copy data
    if (size > 0 && data) {
        msg.data = static_cast<u8 *>(Alloc(ALLOCATOR_DEFAULT, size));
        memcpy(msg.data, data, size);
    } else {
        msg.data = nullptr;
    }

    msg.type = type;
    msg.size = size;

    ws->message_write_index = (ws->message_write_index + 1) % MAX_MESSAGES_PER_SOCKET;
    ws->message_count++;
}

static void ClearMessageQueue(WebWebSocket* ws) {
    while (ws->message_count > 0) {
        WebSocketMessage& msg = ws->messages[ws->message_read_index];
        if (msg.data) {
            Free(msg.data);
            msg.data = nullptr;
        }
        ws->message_read_index = (ws->message_read_index + 1) % MAX_MESSAGES_PER_SOCKET;
        ws->message_count--;
    }
    ws->message_read_index = 0;
    ws->message_write_index = 0;
}

// WebSocket callbacks
static EM_BOOL OnOpen(int event_type, const EmscriptenWebSocketOpenEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    WebWebSocket* ws = GetSocketByEmscriptenHandle(event->socket);
    if (ws) {
        ws->status = WebSocketStatus::Connected;
        LogInfo("WebSocket connected");
    }

    return EM_TRUE;
}

static EM_BOOL OnMessage(int event_type, const EmscriptenWebSocketMessageEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    WebWebSocket* ws = GetSocketByEmscriptenHandle(event->socket);
    if (!ws) return EM_TRUE;

    WebSocketMessageType type = event->isText ? WebSocketMessageType::Text : WebSocketMessageType::Binary;
    QueueMessage(ws, type, event->data, event->numBytes);

    return EM_TRUE;
}

static EM_BOOL OnError(int event_type, const EmscriptenWebSocketErrorEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    WebWebSocket* ws = GetSocketByEmscriptenHandle(event->socket);
    if (ws) {
        ws->status = WebSocketStatus::Error;
        LogError("WebSocket error");
    }

    return EM_TRUE;
}

static EM_BOOL OnClose(int event_type, const EmscriptenWebSocketCloseEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    WebWebSocket* ws = GetSocketByEmscriptenHandle(event->socket);
    if (ws) {
        ws->status = WebSocketStatus::Closed;
        ws->close_code = event->code;
        LogInfo("WebSocket closed (code %d)", event->code);
    }

    return EM_TRUE;
}

void PlatformInitWebSocket() {
    g_ws = {};
    g_ws.next_id = 1;

    // Mark all slots as available (Closed with handle 0)
    for (int i = 0; i < MAX_WEBSOCKETS; i++) {
        g_ws.sockets[i].status = WebSocketStatus::Closed;
        g_ws.sockets[i].handle = 0;
    }

    LogInfo("Web WebSocket initialized");
}

void PlatformShutdownWebSocket() {
    // Clean up all sockets
    for (int i = 0; i < MAX_WEBSOCKETS; i++) {
        if (g_ws.sockets[i].handle > 0) {
            emscripten_websocket_close(g_ws.sockets[i].handle, 1000, "shutdown");
            emscripten_websocket_delete(g_ws.sockets[i].handle);
            ClearMessageQueue(&g_ws.sockets[i]);
        }
    }
}

void PlatformUpdateWebSocket() {
    // No-op for web - callbacks handle updates
}

PlatformWebSocketHandle PlatformConnectWebSocket(const char* url) {
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_WEBSOCKETS; i++) {
        if (g_ws.sockets[i].status == WebSocketStatus::Closed && g_ws.sockets[i].handle == 0) {
            slot = i;
            break;
        }
    }

    if (slot < 0) {
        LogError("WebSocket: No free slots");
        return PlatformWebSocketHandle{0};
    }

    WebWebSocket& ws = g_ws.sockets[slot];
    ws.generation = ++g_ws.next_id;
    ws.status = WebSocketStatus::Connecting;
    ws.close_code = 0;
    ws.message_read_index = 0;
    ws.message_write_index = 0;
    ws.message_count = 0;

    EmscriptenWebSocketCreateAttributes attrs = {
        url,
        nullptr,  // protocols
        EM_TRUE   // createOnMainThread
    };

    ws.handle = emscripten_websocket_new(&attrs);
    if (ws.handle <= 0) {
        LogError("WebSocket: Failed to create connection to %s", url);
        ws.status = WebSocketStatus::Error;
        return PlatformWebSocketHandle{0};
    }

    // Set callbacks
    emscripten_websocket_set_onopen_callback(ws.handle, nullptr, OnOpen);
    emscripten_websocket_set_onmessage_callback(ws.handle, nullptr, OnMessage);
    emscripten_websocket_set_onerror_callback(ws.handle, nullptr, OnError);
    emscripten_websocket_set_onclose_callback(ws.handle, nullptr, OnClose);

    return MakeWebSocketHandle(slot, ws.generation);
}

void PlatformSend(const PlatformWebSocketHandle& handle, const char* text) {
    WebWebSocket* ws = GetSocket(handle);
    if (!ws || ws->status != WebSocketStatus::Connected)
        return;

    emscripten_websocket_send_utf8_text(ws->handle, text);
}

void PlatformSendBinary(const PlatformWebSocketHandle& handle, const void* data, u32 size) {
    WebWebSocket* ws = GetSocket(handle);
    if (!ws || ws->status != WebSocketStatus::Connected)
        return;

    emscripten_websocket_send_binary(ws->handle, (void*)data, size);
}

void PlatformClose(const PlatformWebSocketHandle& handle, u16 code, const char* reason) {
    WebWebSocket* ws = GetSocket(handle);
    if (!ws)
        return;

    if (ws->handle > 0 && ws->status != WebSocketStatus::Closed) {
        emscripten_websocket_close(ws->handle, code, reason ? reason : "");
        ws->status = WebSocketStatus::Closed;
    }
}

void PlatformFree(const PlatformWebSocketHandle& handle) {
    WebWebSocket* ws = GetSocket(handle);
    if (!ws)
        return;

    if (ws->handle > 0) {
        if (ws->status != WebSocketStatus::Closed) {
            emscripten_websocket_close(ws->handle, 1000, "cleanup");
        }
        emscripten_websocket_delete(ws->handle);
    }

    ClearMessageQueue(ws);

    ws->handle = 0;
    ws->status = WebSocketStatus::Closed;
    ws->generation = 0;
}

WebSocketStatus PlatformGetStatus(const PlatformWebSocketHandle& handle) {
    WebWebSocket* ws = GetSocket(handle);
    if (!ws)
        return WebSocketStatus::Closed;

    return ws->status;
}

bool PlatformHasMessages(const PlatformWebSocketHandle& handle) {
    WebWebSocket* ws = GetSocket(handle);
    if (!ws)
        return false;

    return ws->message_count > 0;
}

bool PlatformGetMessage(const PlatformWebSocketHandle& handle, WebSocketMessageType* out_type, u8** out_data, u32* out_size) {
    WebWebSocket* ws = GetSocket(handle);
    if (!ws || ws->message_count == 0)
        return false;

    WebSocketMessage& msg = ws->messages[ws->message_read_index];

    if (out_type) *out_type = msg.type;
    if (out_data) *out_data = msg.data;
    if (out_size) *out_size = msg.size;

    return true;
}

void PlatformPopMessage(const PlatformWebSocketHandle& handle) {
    WebWebSocket* ws = GetSocket(handle);
    if (!ws || ws->message_count == 0)
        return;

    WebSocketMessage& msg = ws->messages[ws->message_read_index];
    if (msg.data) {
        Free(msg.data);
        msg.data = nullptr;
    }

    ws->message_read_index = (ws->message_read_index + 1) % MAX_MESSAGES_PER_SOCKET;
    ws->message_count--;
}

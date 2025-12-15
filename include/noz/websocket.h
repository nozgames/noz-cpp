//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <functional>

struct WebSocket;

enum class WebSocketStatus {
    None,
    Connecting,
    Connected,
    Closing,
    Closed,
    Error
};

enum class WebSocketMessageType {
    Text,
    Binary
};

using WebSocketMessageCallback = std::function<void(WebSocket*, WebSocketMessageType type, const u8* data, u32 size)>;
using WebSocketCloseCallback = std::function<void(WebSocket*, u16 code, const char* reason)>;
using WebSocketConnectCallback = std::function<void(WebSocket*, bool success)>;

enum WebSocketEvent {
    WEBSOCKET_EVENT_CONNECTED,
    WEBSOCKET_EVENT_MESSAGE,
    WEBSOCKET_EVENT_CLOSED
};

typedef void (*WebSocketProc)(WebSocket* socket, WebSocketEvent event, void* event_data, u32 event_data_size, void* user_data);

struct WebSocketEventMessage {
    const u8* data;
    u32 size;
};

// Connect to a WebSocket server
// URL format: ws://host:port/path or wss://host:port/path
WebSocket* CreateWebSocket(Allocator* allocator, const char* url, WebSocketProc proc, void* user_data=nullptr);

// Set callbacks (optional - can also poll)
void WebSocketOnConnect(WebSocket* ws, WebSocketConnectCallback callback);
void WebSocketOnMessage(WebSocket* ws, WebSocketMessageCallback callback);
void WebSocketOnClose(WebSocket* ws, WebSocketCloseCallback callback);

// Query state
WebSocketStatus WebSocketGetStatus(WebSocket* ws);
bool WebSocketIsConnected(WebSocket* ws);
bool WebSocketIsConnecting(WebSocket* ws);

// Send data
void WebSocketSend(WebSocket* ws, const char* text);
void WebSocketSendBinary(WebSocket* ws, const void* data, u32 size);

// Receive data (polling API - alternative to callbacks)
bool WebSocketHasMessage(WebSocket* ws);
bool WebSocketPeekMessage(WebSocket* ws, WebSocketMessageType* out_type, const u8** out_data, u32* out_size);
const char* WebSocketPeekMessageText(WebSocket* ws);  // Convenience for text messages
void WebSocketPopMessage(WebSocket* ws);

// Close connection
void Close(WebSocket* socket, u16 code = 1000, const char* reason = nullptr);
void Free(WebSocket* socket);

// Module init/shutdown (called by engine)
void InitWebSocket();
void ShutdownWebSocket();
void UpdateWebSocket();  // Call each frame to dispatch callbacks

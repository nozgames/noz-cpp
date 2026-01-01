//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

namespace noz {

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

enum WebSocketFlags : u32 {
    WEBSOCKET_FLAG_NONE = 0,
    WEBSOCKET_FLAG_GZIP = 1 << 0,  // Enable automatic gzip decompression of messages
};

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

struct WebSocketConfig {
    Url url;
    WebSocketProc on_event = nullptr;
    void* user_data = nullptr;
    WebSocketFlags flags = WEBSOCKET_FLAG_GZIP;  // Gzip enabled by default
};

// Connect to a WebSocket server
// URL format: ws://host:port/path or wss://host:port/path
extern WebSocket* CreateWebSocket(const WebSocketConfig& config);

// Call each frame to process events and dispatch callbacks
extern void Update(WebSocket* ws);

// Query state
extern WebSocketStatus GetStatus(WebSocket* ws);
extern bool IsConnected(WebSocket* ws);
extern bool IsConnecting(WebSocket* ws);

// Send data
extern void Send(WebSocket* ws, const char* text);
extern void SendBinary(WebSocket* ws, const void* data, u32 size);

// Receive data (polling API - alternative to callbacks)
extern bool HasMessage(WebSocket* ws);
extern bool PeekMessage(WebSocket* ws, WebSocketMessageType* out_type, u8** out_data, u32* out_size);
extern const char* PeekMessageText(WebSocket* ws);  // Convenience for text messages
extern void PopMessage(WebSocket* ws);

// Close connection
extern void Close(WebSocket* socket, u16 code = 1000, const char* reason = nullptr);

}

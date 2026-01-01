//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

namespace noz {

struct WebSocketServer;
struct WebSocketConnection;

using WebSocketServerMessageCallback = std::function<void(WebSocketConnection*, const u8* data, u32 size, bool is_binary)>;
using WebSocketServerConnectCallback = std::function<void(WebSocketConnection*)>;
using WebSocketServerDisconnectCallback = std::function<void(WebSocketConnection*)>;

struct WebSocketServerConfig {
    u16 port = 8080;
    u32 max_connections = 16;
    WebSocketServerConnectCallback on_connect;
    WebSocketServerMessageCallback on_message;
    WebSocketServerDisconnectCallback on_disconnect;
};

extern WebSocketServer* CreateWebSocketServer(const WebSocketServerConfig& config);

extern void DestroyWebSocketServer(WebSocketServer* server);
extern void Update(WebSocketServer* server);
extern void Send(WebSocketConnection* connection, const char* text);
extern void Send(WebSocketConnection* connection, const void* data, u32 size);
extern void Broadcast(WebSocketServer* server, const char* text);
extern void Broadcast(WebSocketServer* server, const void* data, u32 size);
extern void Close(WebSocketConnection* connection);

// Query
extern u32 GetConnectionCount(WebSocketServer* server);
extern bool IsRunning(WebSocketServer* server);
extern int GetId(WebSocketConnection* connection);

}
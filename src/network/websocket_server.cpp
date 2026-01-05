//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "noz/websocket_server.h"

namespace noz {
struct WebSocketServer {
    PlatformWebSocketServerHandle handle;
    bool active;
};

struct WebSocketConnection {
    PlatformWebSocketConnectionHandle handle;
};

constexpr int MAX_SERVERS = 4;
static WebSocketServer g_servers[MAX_SERVERS] = {};

// Adapter to convert platform callbacks to user callbacks
struct CallbackAdapter {
    WebSocketServerConnectCallback on_connect;
    WebSocketServerMessageCallback on_message;
    WebSocketServerDisconnectCallback on_disconnect;
};

static CallbackAdapter g_adapters[MAX_SERVERS] = {};

// Connection wrappers - one per possible connection
constexpr int MAX_CONNECTIONS = 64;
static WebSocketConnection g_connections[MAX_CONNECTIONS] = {};

static WebSocketConnection* WrapConnection(PlatformWebSocketConnectionHandle handle) {
    // Use the connection index from the handle to get a stable pointer
    u32 index = (u32)(handle.value & 0xFFFFFFFF);
    if (index >= MAX_CONNECTIONS) return nullptr;
    g_connections[index].handle = handle;
    return &g_connections[index];
}

WebSocketServer* CreateWebSocketServer(const WebSocketServerConfig& config) {
    // Find a free server slot
    int server_index = -1;
    for (int i = 0; i < MAX_SERVERS; i++) {
        if (!g_servers[i].active) {
            server_index = i;
            break;
        }
    }

    if (server_index < 0) {
        LogWarning("No free WebSocket server slots");
        return nullptr;
    }

    // Store user callbacks
    g_adapters[server_index].on_connect = config.on_connect;
    g_adapters[server_index].on_message = config.on_message;
    g_adapters[server_index].on_disconnect = config.on_disconnect;

    // Create platform config with wrapper callbacks
    PlatformWebSocketServerConfig platform_config;
    platform_config.port = config.port;
    platform_config.max_connections = config.max_connections;

    int idx = server_index;  // Capture for lambdas

    platform_config.on_connect = [idx](PlatformWebSocketConnectionHandle conn) {
        if (g_adapters[idx].on_connect) {
            g_adapters[idx].on_connect(WrapConnection(conn));
        }
    };

    platform_config.on_message = [idx](PlatformWebSocketConnectionHandle conn, const u8* data, u32 size, bool is_binary) {
        if (g_adapters[idx].on_message) {
            g_adapters[idx].on_message(WrapConnection(conn), data, size, is_binary);
        }
    };

    platform_config.on_disconnect = [idx](PlatformWebSocketConnectionHandle conn) {
        if (g_adapters[idx].on_disconnect) {
            g_adapters[idx].on_disconnect(WrapConnection(conn));
        }
    };

    PlatformWebSocketServerHandle handle = PlatformCreateWebSocketServer(platform_config);
    if (handle.value == 0) {
        return nullptr;
    }

    g_servers[server_index].handle = handle;
    g_servers[server_index].active = true;

    return &g_servers[server_index];
}

void DestroyWebSocketServer(WebSocketServer* server) {
    if (!server || !server->active) return;

    PlatformDestroyWebSocketServer(server->handle);
    server->active = false;
}

void Update(WebSocketServer* server) {
    if (!server || !server->active) return;

    PlatformUpdateWebSocketServer(server->handle);
}

void Send(WebSocketConnection* connection, const char* text) {
    if (!connection) return;
    PlatformWebSocketServerSend(connection->handle, text);
}

void Send(WebSocketConnection* connection, const void* data, u32 size) {
    if (!connection) return;
    PlatformWebSocketServerSendBinary(connection->handle, data, size);
}

void Broadcast(WebSocketServer* server, const char* text) {
    if (!server || !server->active) return;
    PlatformWebSocketServerBroadcast(server->handle, text);
}

void Broadcast(WebSocketServer* server, const void* data, u32 size) {
    if (!server || !server->active) return;
    PlatformWebSocketServerBroadcastBinary(server->handle, data, size);
}

void Close(WebSocketConnection* connection) {
    if (!connection) return;
    PlatformWebSocketServerClose(connection->handle);
}

u32 GetConnectionCount(WebSocketServer* server) {
    if (!server || !server->active) return 0;
    return PlatformWebSocketServerGetConnectionCount(server->handle);
}

bool IsRunning(WebSocketServer* server) {
    if (!server || !server->active) return false;
    return PlatformWebSocketServerIsRunning(server->handle);
}

int GetId(WebSocketConnection* connection) {
    if (!connection) return 0;
    return (int)PlatformWebSocketServerGetConnectionId(connection->handle);
}

}

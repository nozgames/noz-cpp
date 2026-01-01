//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include <vector>
#include <mutex>
#include <algorithm>

constexpr u32 MAX_SERVERS = 4;
constexpr u32 MAX_CONNECTIONS = 64;
constexpr u32 RECV_BUFFER_SIZE = 16 * 1024;

// Simple SHA1 implementation for WebSocket handshake
namespace {
    struct SHA1Context {
        u32 state[5];
        u32 count[2];
        u8 buffer[64];
    };

    static void SHA1Transform(u32 state[5], const u8 buffer[64]) {
        u32 a, b, c, d, e, w[80];

        auto rol = [](u32 value, u32 bits) { return (value << bits) | (value >> (32 - bits)); };

        for (int i = 0; i < 16; i++) {
            w[i] = ((u32)buffer[i * 4] << 24) | ((u32)buffer[i * 4 + 1] << 16) |
                   ((u32)buffer[i * 4 + 2] << 8) | buffer[i * 4 + 3];
        }
        for (int i = 16; i < 80; i++) {
            w[i] = rol(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
        }

        a = state[0]; b = state[1]; c = state[2]; d = state[3]; e = state[4];

        for (int i = 0; i < 80; i++) {
            u32 f, k;
            if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }

            u32 temp = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = temp;
        }

        state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
    }

    static void SHA1Init(SHA1Context* ctx) {
        ctx->state[0] = 0x67452301; ctx->state[1] = 0xEFCDAB89;
        ctx->state[2] = 0x98BADCFE; ctx->state[3] = 0x10325476;
        ctx->state[4] = 0xC3D2E1F0;
        ctx->count[0] = ctx->count[1] = 0;
    }

    static void SHA1Update(SHA1Context* ctx, const u8* data, size_t len) {
        size_t i, j = (ctx->count[0] >> 3) & 63;
        if ((ctx->count[0] += (u32)(len << 3)) < (len << 3)) ctx->count[1]++;
        ctx->count[1] += (u32)(len >> 29);

        if ((j + len) > 63) {
            memcpy(&ctx->buffer[j], data, (i = 64 - j));
            SHA1Transform(ctx->state, ctx->buffer);
            for (; i + 63 < len; i += 64) SHA1Transform(ctx->state, &data[i]);
            j = 0;
        } else i = 0;
        memcpy(&ctx->buffer[j], &data[i], len - i);
    }

    static void SHA1Final(u8 digest[20], SHA1Context* ctx) {
        u8 finalcount[8];
        for (int i = 0; i < 8; i++) {
            finalcount[i] = (u8)((ctx->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255);
        }
        u8 c = 0200;
        SHA1Update(ctx, &c, 1);
        while ((ctx->count[0] & 504) != 448) { c = 0; SHA1Update(ctx, &c, 1); }
        SHA1Update(ctx, finalcount, 8);
        for (int i = 0; i < 20; i++) {
            digest[i] = (u8)((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
        }
    }
}

// WebSocket frame opcodes
enum WebSocketOpcode : u8 {
    WS_OPCODE_CONTINUATION = 0x0,
    WS_OPCODE_TEXT = 0x1,
    WS_OPCODE_BINARY = 0x2,
    WS_OPCODE_CLOSE = 0x8,
    WS_OPCODE_PING = 0x9,
    WS_OPCODE_PONG = 0xA
};

struct WindowsWebSocketConnection {
    SOCKET socket = INVALID_SOCKET;
    u32 server_index = 0;
    bool handshake_complete = false;
    bool should_close = false;
    u32 id = 0;
    u32 generation = 0;
    bool in_use = false;

    // Receive buffer for partial data
    std::vector<u8> recv_buffer;

    // Frame assembly buffer
    std::vector<u8> frame_buffer;
    WebSocketOpcode frame_opcode = WS_OPCODE_TEXT;
};

struct WindowsWebSocketServer {
    SOCKET listen_socket = INVALID_SOCKET;
    std::vector<u32> connection_indices;
    PlatformWebSocketServerConfig config;
    bool running = false;
    u32 next_conn_id = 1;
    u32 generation = 0;
    bool in_use = false;
    std::mutex mutex;
};

struct WindowsWebSocketServerState {
    WindowsWebSocketServer servers[MAX_SERVERS];
    WindowsWebSocketConnection connections[MAX_CONNECTIONS];
    bool wsa_initialized = false;
};

static WindowsWebSocketServerState g_wss = {};

static void InitWSA() {
    if (!g_wss.wsa_initialized) {
        WSADATA wsa_data;
        WSAStartup(MAKEWORD(2, 2), &wsa_data);
        g_wss.wsa_initialized = true;
    }
}

static u32 GetServerIndex(const PlatformWebSocketServerHandle& handle) {
    return (u32)(handle.value & 0xFFFFFFFF);
}

static u32 GetServerGeneration(const PlatformWebSocketServerHandle& handle) {
    return (u32)(handle.value >> 32);
}

static PlatformWebSocketServerHandle MakeServerHandle(u32 index, u32 generation) {
    PlatformWebSocketServerHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

static WindowsWebSocketServer* GetServer(const PlatformWebSocketServerHandle& handle) {
    u32 index = GetServerIndex(handle);
    u32 generation = GetServerGeneration(handle);

    if (index >= MAX_SERVERS)
        return nullptr;

    WindowsWebSocketServer& server = g_wss.servers[index];
    if (!server.in_use || server.generation != generation)
        return nullptr;

    return &server;
}

static u32 GetConnectionIndex(const PlatformWebSocketConnectionHandle& handle) {
    return (u32)(handle.value & 0xFFFFFFFF);
}

static u32 GetConnectionGeneration(const PlatformWebSocketConnectionHandle& handle) {
    return (u32)(handle.value >> 32);
}

static PlatformWebSocketConnectionHandle MakeConnectionHandle(u32 index, u32 generation) {
    PlatformWebSocketConnectionHandle handle;
    handle.value = ((u64)generation << 32) | (u64)index;
    return handle;
}

static WindowsWebSocketConnection* GetConnection(const PlatformWebSocketConnectionHandle& handle) {
    u32 index = GetConnectionIndex(handle);
    u32 generation = GetConnectionGeneration(handle);

    if (index >= MAX_CONNECTIONS)
        return nullptr;

    WindowsWebSocketConnection& conn = g_wss.connections[index];
    if (!conn.in_use || conn.generation != generation)
        return nullptr;

    return &conn;
}

static std::string Base64Encode(const u8* data, size_t len) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve((len + 2) / 3 * 4);

    for (size_t i = 0; i < len; i += 3) {
        u32 n = (u32)data[i] << 16;
        if (i + 1 < len) n |= (u32)data[i + 1] << 8;
        if (i + 2 < len) n |= data[i + 2];

        result += chars[(n >> 18) & 0x3F];
        result += chars[(n >> 12) & 0x3F];
        result += (i + 1 < len) ? chars[(n >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? chars[n & 0x3F] : '=';
    }
    return result;
}

static std::string GenerateAcceptKey(const std::string& client_key) {
    const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = client_key + magic;

    SHA1Context ctx;
    SHA1Init(&ctx);
    SHA1Update(&ctx, (const u8*)combined.c_str(), combined.length());
    u8 hash[20];
    SHA1Final(hash, &ctx);

    return Base64Encode(hash, 20);
}

static bool ParseHandshake(const char* data, size_t len, std::string& out_key) {
    std::string request(data, len);

    if (request.find("GET ") != 0) return false;

    size_t key_pos = request.find("Sec-WebSocket-Key: ");
    if (key_pos == std::string::npos) return false;

    key_pos += 19;
    size_t key_end = request.find("\r\n", key_pos);
    if (key_end == std::string::npos) return false;

    out_key = request.substr(key_pos, key_end - key_pos);
    return true;
}

static bool SendHandshakeResponse(SOCKET socket, const std::string& accept_key) {
    std::string response =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept_key + "\r\n"
        "\r\n";

    int sent = send(socket, response.c_str(), (int)response.length(), 0);
    return sent == (int)response.length();
}

static bool SendFrame(SOCKET socket, WebSocketOpcode opcode, const void* data, u32 size) {
    std::vector<u8> frame;

    // First byte: FIN + opcode
    frame.push_back(0x80 | opcode);

    // Payload length (server frames are NOT masked)
    if (size < 126) {
        frame.push_back((u8)size);
    } else if (size < 65536) {
        frame.push_back(126);
        frame.push_back((u8)(size >> 8));
        frame.push_back((u8)(size & 0xFF));
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((u8)((size >> (i * 8)) & 0xFF));
        }
    }

    // Payload
    const u8* payload = (const u8*)data;
    frame.insert(frame.end(), payload, payload + size);

    int sent = send(socket, (const char*)frame.data(), (int)frame.size(), 0);
    return sent == (int)frame.size();
}

// Process incoming data for a connection
static void ProcessConnectionData(WindowsWebSocketConnection* conn, WindowsWebSocketServer* server) {
    if (conn->recv_buffer.empty()) return;

    // Handle handshake first
    if (!conn->handshake_complete) {
        // Look for end of HTTP headers
        std::string data((char*)conn->recv_buffer.data(), conn->recv_buffer.size());
        size_t header_end = data.find("\r\n\r\n");
        if (header_end == std::string::npos) return;  // Need more data

        std::string client_key;
        if (!ParseHandshake(data.c_str(), header_end + 4, client_key)) {
            conn->should_close = true;
            return;
        }

        std::string accept_key = GenerateAcceptKey(client_key);
        if (!SendHandshakeResponse(conn->socket, accept_key)) {
            conn->should_close = true;
            return;
        }

        conn->handshake_complete = true;
        conn->recv_buffer.erase(conn->recv_buffer.begin(), conn->recv_buffer.begin() + header_end + 4);

        // Notify connect
        if (server->config.on_connect) {
            server->config.on_connect(MakeConnectionHandle(conn->id, conn->generation));
        }
    }

    // Process WebSocket frames
    while (conn->recv_buffer.size() >= 2) {
        const u8* data = conn->recv_buffer.data();
        size_t available = conn->recv_buffer.size();

        bool fin = (data[0] & 0x80) != 0;
        WebSocketOpcode opcode = (WebSocketOpcode)(data[0] & 0x0F);
        bool masked = (data[1] & 0x80) != 0;
        u64 payload_len = data[1] & 0x7F;

        size_t header_size = 2;

        if (payload_len == 126) {
            if (available < 4) return;  // Need more data
            payload_len = ((u64)data[2] << 8) | data[3];
            header_size = 4;
        } else if (payload_len == 127) {
            if (available < 10) return;  // Need more data
            payload_len = 0;
            for (int i = 0; i < 8; i++) {
                payload_len = (payload_len << 8) | data[2 + i];
            }
            header_size = 10;
        }

        if (masked) header_size += 4;

        if (available < header_size + payload_len) return;  // Need more data

        // Extract mask key if present
        u8 mask_key[4] = {0, 0, 0, 0};
        if (masked) {
            memcpy(mask_key, data + header_size - 4, 4);
        }

        // Extract and unmask payload
        std::vector<u8> payload(payload_len);
        const u8* payload_data = data + header_size;
        for (u64 i = 0; i < payload_len; i++) {
            payload[i] = payload_data[i] ^ mask_key[i % 4];
        }

        // Remove processed data from buffer
        conn->recv_buffer.erase(conn->recv_buffer.begin(), conn->recv_buffer.begin() + header_size + payload_len);

        // Handle frame
        if (opcode == WS_OPCODE_CLOSE) {
            // Send close frame back
            SendFrame(conn->socket, WS_OPCODE_CLOSE, nullptr, 0);
            conn->should_close = true;
            return;
        } else if (opcode == WS_OPCODE_PING) {
            // Respond with pong
            SendFrame(conn->socket, WS_OPCODE_PONG, payload.data(), (u32)payload.size());
        } else if (opcode == WS_OPCODE_PONG) {
            // Ignore pongs
        } else if (opcode == WS_OPCODE_TEXT || opcode == WS_OPCODE_BINARY || opcode == WS_OPCODE_CONTINUATION) {
            // Data frame
            if (opcode != WS_OPCODE_CONTINUATION) {
                conn->frame_opcode = opcode;
                conn->frame_buffer.clear();
            }

            conn->frame_buffer.insert(conn->frame_buffer.end(), payload.begin(), payload.end());

            if (fin) {
                // Complete message
                if (server->config.on_message) {
                    bool is_binary = (conn->frame_opcode == WS_OPCODE_BINARY);
                    // Find the connection index for this connection
                    u32 conn_index = 0;
                    for (u32 i = 0; i < MAX_CONNECTIONS; i++) {
                        if (&g_wss.connections[i] == conn) {
                            conn_index = i;
                            break;
                        }
                    }
                    server->config.on_message(MakeConnectionHandle(conn_index, conn->generation),
                                             conn->frame_buffer.data(), (u32)conn->frame_buffer.size(), is_binary);
                }
                conn->frame_buffer.clear();
            }
        }
    }
}

PlatformWebSocketServerHandle PlatformCreateWebSocketServer(const PlatformWebSocketServerConfig& config) {
    InitWSA();

    // Find a free server slot
    u32 server_index = MAX_SERVERS;
    for (u32 i = 0; i < MAX_SERVERS; i++) {
        if (!g_wss.servers[i].in_use) {
            server_index = i;
            break;
        }
    }

    if (server_index >= MAX_SERVERS) {
        return PlatformWebSocketServerHandle{0};
    }

    WindowsWebSocketServer& server = g_wss.servers[server_index];
    server.config = config;
    server.in_use = true;
    server.generation++;

    // Create listening socket
    server.listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server.listen_socket == INVALID_SOCKET) {
        server.in_use = false;
        return PlatformWebSocketServerHandle{0};
    }

    // Allow address reuse
    int opt = 1;
    setsockopt(server.listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    // Bind
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(config.port);

    if (bind(server.listen_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(server.listen_socket);
        server.in_use = false;
        return PlatformWebSocketServerHandle{0};
    }

    // Listen
    if (listen(server.listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(server.listen_socket);
        server.in_use = false;
        return PlatformWebSocketServerHandle{0};
    }

    // Set non-blocking
    u_long mode = 1;
    ioctlsocket(server.listen_socket, FIONBIO, &mode);

    server.running = true;

    return MakeServerHandle(server_index, server.generation);
}

void PlatformDestroyWebSocketServer(const PlatformWebSocketServerHandle& handle) {
    WindowsWebSocketServer* server = GetServer(handle);
    if (!server) return;

    std::lock_guard lock(server->mutex);

    // Close all connections
    for (u32 conn_index : server->connection_indices) {
        WindowsWebSocketConnection& conn = g_wss.connections[conn_index];
        if (conn.socket != INVALID_SOCKET) {
            closesocket(conn.socket);
        }
        conn = WindowsWebSocketConnection{};
    }
    server->connection_indices.clear();

    if (server->listen_socket != INVALID_SOCKET)
        closesocket(server->listen_socket);

    server->running = false;
    server->in_use = false;
}

void PlatformUpdateWebSocketServer(const PlatformWebSocketServerHandle& handle) {
    WindowsWebSocketServer* server = GetServer(handle);
    if (!server || !server->running) return;

    std::lock_guard lock(server->mutex);

    u32 server_index = GetServerIndex(handle);

    // Accept new connections
    while (true) {
        sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
        SOCKET client_socket = accept(server->listen_socket, (sockaddr*)&client_addr, &addr_len);

        if (client_socket == INVALID_SOCKET) break;

        if (server->connection_indices.size() >= server->config.max_connections) {
            closesocket(client_socket);
            continue;
        }

        // Find a free connection slot
        u32 conn_index = MAX_CONNECTIONS;
        for (u32 i = 0; i < MAX_CONNECTIONS; i++) {
            if (!g_wss.connections[i].in_use) {
                conn_index = i;
                break;
            }
        }

        if (conn_index >= MAX_CONNECTIONS) {
            closesocket(client_socket);
            continue;
        }

        // Set non-blocking
        u_long mode = 1;
        ioctlsocket(client_socket, FIONBIO, &mode);

        // Disable Nagle's algorithm for lower latency
        int nodelay = 1;
        setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&nodelay, sizeof(nodelay));

        WindowsWebSocketConnection& conn = g_wss.connections[conn_index];
        conn = WindowsWebSocketConnection{};
        conn.socket = client_socket;
        conn.server_index = server_index;
        conn.id = server->next_conn_id++;
        conn.generation++;
        conn.in_use = true;

        server->connection_indices.push_back(conn_index);
    }

    // Process existing connections
    std::vector<u32> to_remove;

    for (u32 conn_index : server->connection_indices) {
        WindowsWebSocketConnection& conn = g_wss.connections[conn_index];

        if (conn.should_close) {
            to_remove.push_back(conn_index);
            continue;
        }

        // Receive data
        u8 buffer[RECV_BUFFER_SIZE];
        int received = recv(conn.socket, (char*)buffer, sizeof(buffer), 0);

        if (received > 0) {
            conn.recv_buffer.insert(conn.recv_buffer.end(), buffer, buffer + received);
            ProcessConnectionData(&conn, server);
        } else if (received == 0) {
            // Connection closed
            conn.should_close = true;
            to_remove.push_back(conn_index);
        } else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                conn.should_close = true;
                to_remove.push_back(conn_index);
            }
        }
    }

    // Remove closed connections
    for (u32 conn_index : to_remove) {
        WindowsWebSocketConnection& conn = g_wss.connections[conn_index];

        if (conn.handshake_complete && server->config.on_disconnect) {
            server->config.on_disconnect(MakeConnectionHandle(conn_index, conn.generation));
        }

        if (conn.socket != INVALID_SOCKET)
            closesocket(conn.socket);

        auto it = std::find(server->connection_indices.begin(), server->connection_indices.end(), conn_index);
        if (it != server->connection_indices.end())
            server->connection_indices.erase(it);

        conn.in_use = false;
    }
}

void PlatformWebSocketServerSend(const PlatformWebSocketConnectionHandle& connection, const char* text) {
    WindowsWebSocketConnection* conn = GetConnection(connection);
    if (!conn || conn->should_close || !conn->handshake_complete) return;
    SendFrame(conn->socket, WS_OPCODE_TEXT, text, (u32)strlen(text));
}

void PlatformWebSocketServerSendBinary(const PlatformWebSocketConnectionHandle& connection, const void* data, u32 size) {
    WindowsWebSocketConnection* conn = GetConnection(connection);
    if (!conn || conn->should_close || !conn->handshake_complete) return;
    SendFrame(conn->socket, WS_OPCODE_BINARY, data, size);
}

void PlatformWebSocketServerBroadcast(const PlatformWebSocketServerHandle& handle, const char* text) {
    WindowsWebSocketServer* server = GetServer(handle);
    if (!server) return;
    std::lock_guard lock(server->mutex);

    for (u32 conn_index : server->connection_indices) {
        WindowsWebSocketConnection& conn = g_wss.connections[conn_index];
        if (conn.handshake_complete && !conn.should_close) {
            SendFrame(conn.socket, WS_OPCODE_TEXT, text, (u32)strlen(text));
        }
    }
}

void PlatformWebSocketServerBroadcastBinary(const PlatformWebSocketServerHandle& handle, const void* data, u32 size) {
    WindowsWebSocketServer* server = GetServer(handle);
    if (!server) return;
    std::lock_guard lock(server->mutex);

    for (u32 conn_index : server->connection_indices) {
        WindowsWebSocketConnection& conn = g_wss.connections[conn_index];
        if (conn.handshake_complete && !conn.should_close) {
            SendFrame(conn.socket, WS_OPCODE_BINARY, data, size);
        }
    }
}

void PlatformWebSocketServerClose(const PlatformWebSocketConnectionHandle& connection) {
    WindowsWebSocketConnection* conn = GetConnection(connection);
    if (!conn) return;
    SendFrame(conn->socket, WS_OPCODE_CLOSE, nullptr, 0);
    conn->should_close = true;
}

u32 PlatformWebSocketServerGetConnectionCount(const PlatformWebSocketServerHandle& handle) {
    WindowsWebSocketServer* server = GetServer(handle);
    if (!server) return 0;
    std::lock_guard lock(server->mutex);

    u32 count = 0;
    for (u32 conn_index : server->connection_indices) {
        WindowsWebSocketConnection& conn = g_wss.connections[conn_index];
        if (conn.handshake_complete && !conn.should_close) {
            count++;
        }
    }
    return count;
}

bool PlatformWebSocketServerIsRunning(const PlatformWebSocketServerHandle& handle) {
    WindowsWebSocketServer* server = GetServer(handle);
    return server && server->running;
}

u32 PlatformWebSocketServerGetConnectionId(const PlatformWebSocketConnectionHandle& connection) {
    WindowsWebSocketConnection* conn = GetConnection(connection);
    return conn ? conn->id : 0;
}

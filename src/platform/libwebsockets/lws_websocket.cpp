//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  libwebsockets WebSocket implementation
//

// Manual includes since we skip PCH for this file
#include <noz/noz.h>
#include "../../internal.h"
#include "../../platform.h"
#include <libwebsockets.h>
#include <string.h>
#include <stdlib.h>

constexpr int MAX_WEBSOCKETS = 8;
constexpr int MAX_MESSAGES_PER_SOCKET = 32;
constexpr u32 MAX_MESSAGE_SIZE = 1024 * 1024;  // 1MB per message
constexpr u32 RECEIVE_BUFFER_SIZE = 16 * 1024;

struct WebSocketMessage {
    WebSocketMessageType type;
    u8* data;
    u32 size;
};

struct LWSWebSocket {
    struct lws* wsi;
    struct lws_context* context;
    WebSocketStatus status;
    u64 generation;
    bool should_close;
    u16 close_code;
    char close_reason[128];
    char url[512];

    // Message queue
    WebSocketMessage messages[MAX_MESSAGES_PER_SOCKET];
    int message_read_index;
    int message_write_index;
    int message_count;

    // Receive buffer for fragmented messages
    u8* receive_buffer;
    u32 receive_size;
    u32 receive_capacity;
    WebSocketMessageType receive_type;

    // Thread handle
    void* thread_handle;
    volatile bool thread_running;

    // Critical section for thread safety
    void* cs;  // Platform-specific critical section
};

struct LWSWebSocketState {
    LWSWebSocket sockets[MAX_WEBSOCKETS];
    u64 next_id;
    bool initialized;
};

static LWSWebSocketState g_ws = {};

// Forward declarations
static void QueueMessage(LWSWebSocket* ws, WebSocketMessageType type, const u8* data, u32 size);
static void AppendToReceiveBuffer(LWSWebSocket* ws, const u8* data, u32 size);

// Platform critical section helpers
#ifdef _WIN32
#include <windows.h>
static void* CreateCS() {
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(cs);
    return cs;
}
static void DeleteCS(void* cs) {
    DeleteCriticalSection((CRITICAL_SECTION*)cs);
    free(cs);
}
static void EnterCS(void* cs) { EnterCriticalSection((CRITICAL_SECTION*)cs); }
static void LeaveCS(void* cs) { LeaveCriticalSection((CRITICAL_SECTION*)cs); }
#else
#include <pthread.h>
static void* CreateCS() {
    pthread_mutex_t* mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex, nullptr);
    return mutex;
}
static void DeleteCS(void* cs) {
    pthread_mutex_destroy((pthread_mutex_t*)cs);
    free(cs);
}
static void EnterCS(void* cs) { pthread_mutex_lock((pthread_mutex_t*)cs); }
static void LeaveCS(void* cs) { pthread_mutex_unlock((pthread_mutex_t*)cs); }
#endif

static PlatformWebSocketHandle MakeWebSocketHandle(int index, u64 generation) {
    return PlatformWebSocketHandle{ ((u64)index) | (generation << 32) };
}

static LWSWebSocket* GetSocket(const PlatformWebSocketHandle& handle) {
    int index = (int)(handle.value & 0xFFFFFFFF);
    u64 generation = handle.value >> 32;

    if (index < 0 || index >= MAX_WEBSOCKETS)
        return nullptr;

    LWSWebSocket* ws = &g_ws.sockets[index];
    if (ws->generation != generation)
        return nullptr;

    return ws;
}

static void QueueMessage(LWSWebSocket* ws, WebSocketMessageType type, const u8* data, u32 size) {
    EnterCS(ws->cs);

    if (ws->message_count < MAX_MESSAGES_PER_SOCKET) {
        WebSocketMessage& msg = ws->messages[ws->message_write_index];
        msg.type = type;
        msg.data = (u8*)Alloc(ALLOCATOR_DEFAULT, size + 1);
        memcpy(msg.data, data, size);
        msg.data[size] = '\0';
        msg.size = size;

        ws->message_write_index = (ws->message_write_index + 1) % MAX_MESSAGES_PER_SOCKET;
        ws->message_count++;
    } else {
        LogWarning("WebSocket message queue full, dropping message");
    }

    LeaveCS(ws->cs);
}

static void AppendToReceiveBuffer(LWSWebSocket* ws, const u8* data, u32 size) {
    u32 new_size = ws->receive_size + size;
    if (new_size > ws->receive_capacity) {
        u32 new_capacity = Max(ws->receive_capacity * 2, new_size + RECEIVE_BUFFER_SIZE);
        new_capacity = Min(new_capacity, MAX_MESSAGE_SIZE);

        if (new_size > new_capacity) {
            LogWarning("WebSocket message too large, truncating");
            size = new_capacity - ws->receive_size;
            new_size = new_capacity;
        }

        u8* new_buffer = (u8*)Alloc(ALLOCATOR_DEFAULT, new_capacity);
        if (ws->receive_buffer) {
            memcpy(new_buffer, ws->receive_buffer, ws->receive_size);
            Free(ws->receive_buffer);
        }
        ws->receive_buffer = new_buffer;
        ws->receive_capacity = new_capacity;
    }

    memcpy(ws->receive_buffer + ws->receive_size, data, size);
    ws->receive_size = new_size;
}

// libwebsockets callback
static int lws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len) {
    (void)user;  // Unused
    LWSWebSocket* ws = (LWSWebSocket*)lws_wsi_user(wsi);

    if (!ws) return 0;

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            LogInfo("WebSocket connected");
            EnterCS(ws->cs);
            ws->status = WebSocketStatus::Connected;
            LeaveCS(ws->cs);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            const u8* data = (const u8*)in;
            size_t remaining = lws_remaining_packet_payload(wsi);
            bool is_final = lws_is_final_fragment(wsi);
            bool is_binary = lws_frame_is_binary(wsi);

            LogInfo("Received: len=%zu, remaining=%zu, is_final=%d, is_binary=%d",
                    len, remaining, is_final, is_binary);

            WebSocketMessageType msg_type = is_binary ? WebSocketMessageType::Binary : WebSocketMessageType::Text;

            if (ws->receive_size == 0) {
                // First fragment
                ws->receive_type = msg_type;
            }

            AppendToReceiveBuffer(ws, data, (u32)len);

            if (is_final && remaining == 0) {
                // Complete message
                LogInfo("Complete message assembled: %u bytes", ws->receive_size);

                // Log first 8 bytes to check for gzip header (1f 8b)
                if (ws->receive_size >= 8) {
                    LogInfo("Message first bytes: %02x %02x %02x %02x %02x %02x %02x %02x",
                        ws->receive_buffer[0], ws->receive_buffer[1], ws->receive_buffer[2], ws->receive_buffer[3],
                        ws->receive_buffer[4], ws->receive_buffer[5], ws->receive_buffer[6], ws->receive_buffer[7]);
                }

                QueueMessage(ws, ws->receive_type, ws->receive_buffer, ws->receive_size);
                ws->receive_size = 0;
            }
            break;
        }

        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CLOSED:
            LogInfo("WebSocket closed");
            EnterCS(ws->cs);
            ws->status = WebSocketStatus::Closed;
            LeaveCS(ws->cs);
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            if (in) {
                LogError("WebSocket connection error: %s", (const char*)in);
            } else {
                LogError("WebSocket connection error (no details)");
            }
            EnterCS(ws->cs);
            ws->status = WebSocketStatus::Error;
            LeaveCS(ws->cs);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            // Can send data now (for flow control)
            break;

        case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION:
            // Skip certificate verification - return 0 to accept
            LogInfo("Skipping certificate verification");
            return 0;

        default:
            break;
    }

    return 0;
}

static const struct lws_protocols protocols[] = {
    {
        "default",
        lws_callback,
        0,
        RECEIVE_BUFFER_SIZE,
        0, NULL, 0
    },
    { NULL, NULL, 0, 0, 0, NULL, 0 }
};

// Service thread
#ifdef _WIN32
static DWORD WINAPI ServiceThread(LPVOID param) {
#else
static void* ServiceThread(void* param) {
#endif
    LWSWebSocket* ws = (LWSWebSocket*)param;

    LogInfo("Service thread started");

    while (ws->thread_running && ws->context) {
        lws_service(ws->context, 50);  // 50ms timeout
    }

    LogInfo("Service thread exiting");

#ifdef _WIN32
    return 0;
#else
    return nullptr;
#endif
}

// Platform API implementation

void PlatformInitWebSocket() {
    if (g_ws.initialized)
        return;

    memset(&g_ws, 0, sizeof(g_ws));
    g_ws.initialized = true;
    g_ws.next_id = 1;

    // Enable verbose logging for debugging
    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO, nullptr);
    LogInfo("libwebsockets initialized");
}

void PlatformShutdownWebSocket() {
    if (!g_ws.initialized)
        return;

    // Close all sockets
    for (int i = 0; i < MAX_WEBSOCKETS; i++) {
        LWSWebSocket* ws = &g_ws.sockets[i];
        if (ws->status != WebSocketStatus::None) {
            PlatformFree(MakeWebSocketHandle(i, ws->generation));
        }
    }

    g_ws.initialized = false;
}

void PlatfrormUpdateWebSocket() {
    // libwebsockets uses a service thread, so no update needed
}

PlatformWebSocketHandle PlatformConnectWebSocket(const char* url) {
    // Find free socket slot
    int socket_index = -1;
    for (int i = 0; i < MAX_WEBSOCKETS; i++) {
        if (g_ws.sockets[i].status == WebSocketStatus::None) {
            socket_index = i;
            break;
        }
    }

    if (socket_index == -1) {
        LogWarning("No free WebSocket slots");
        return MakeWebSocketHandle(-1, 0);
    }

    LWSWebSocket* ws = &g_ws.sockets[socket_index];
    memset(ws, 0, sizeof(LWSWebSocket));

    ws->cs = CreateCS();
    ws->status = WebSocketStatus::Connecting;
    ws->generation = ++g_ws.next_id;
    strncpy(ws->url, url, sizeof(ws->url) - 1);

    // Parse URL
    struct lws_client_connect_info ccinfo = {};
    const char* protocol_start = strstr(url, "://");
    bool secure = (strncmp(url, "wss://", 6) == 0);

    if (!protocol_start) {
        LogError("Invalid WebSocket URL");
        ws->status = WebSocketStatus::Error;
        return MakeWebSocketHandle(socket_index, ws->generation);
    }

    const char* host_start = protocol_start + 3;
    const char* path_start = strchr(host_start, '/');
    const char* port_start = strchr(host_start, ':');

    char hostname[256] = {};
    if (port_start && (!path_start || port_start < path_start)) {
        size_t host_len = port_start - host_start;
        strncpy(hostname, host_start, Min(host_len, sizeof(hostname) - 1));
        ccinfo.port = atoi(port_start + 1);
    } else if (path_start) {
        size_t host_len = path_start - host_start;
        strncpy(hostname, host_start, Min(host_len, sizeof(hostname) - 1));
        ccinfo.port = secure ? 443 : 80;
    } else {
        strncpy(hostname, host_start, sizeof(hostname) - 1);
        ccinfo.port = secure ? 443 : 80;
    }

    const char* path = path_start ? path_start : "/";

    // Create context
    struct lws_context_creation_info info = {};
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
#ifndef _WIN32
    info.gid = (gid_t)-1;
    info.uid = (uid_t)-1;
#endif
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    // Skip certificate verification
    info.options |= LWS_SERVER_OPTION_PEER_CERT_NOT_REQUIRED;

    ws->context = lws_create_context(&info);
    if (!ws->context) {
        LogError("Failed to create lws context");
        ws->status = WebSocketStatus::Error;
        DeleteCS(ws->cs);
        return MakeWebSocketHandle(socket_index, ws->generation);
    }

    // Connect
    ccinfo.context = ws->context;
    ccinfo.address = hostname;
    ccinfo.path = path;
    ccinfo.host = hostname;
    ccinfo.origin = hostname;
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = secure ? (LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK | LCCSCF_ALLOW_INSECURE) : 0;
    ccinfo.userdata = ws;
    ccinfo.ietf_version_or_minus_one = -1;  // Use latest WebSocket version

    LogInfo("Connecting to WebSocket: %s:%d%s (secure=%d)", hostname, ccinfo.port, path, secure);

    ws->wsi = lws_client_connect_via_info(&ccinfo);

    LogInfo("lws_client_connect_via_info returned: %p", ws->wsi);

    if (!ws->wsi) {
        LogError("Failed to connect WebSocket to %s - lws_client_connect_via_info returned NULL", url);
        ws->status = WebSocketStatus::Error;
        lws_context_destroy(ws->context);
        ws->context = nullptr;
        DeleteCS(ws->cs);
        return MakeWebSocketHandle(socket_index, ws->generation);
    }

    // Start service thread
    ws->thread_running = true;
#ifdef _WIN32
    ws->thread_handle = CreateThread(nullptr, 0, ServiceThread, ws, 0, nullptr);
#else
    pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t));
    pthread_create(thread, nullptr, ServiceThread, ws);
    ws->thread_handle = thread;
#endif

    return MakeWebSocketHandle(socket_index, ws->generation);
}

void PlatformSend(const PlatformWebSocketHandle& handle, const char* text) {
    LWSWebSocket* ws = GetSocket(handle);
    if (!ws || !ws->wsi) return;

    size_t len = strlen(text);
    u8* buf = static_cast<u8 *>(Alloc(ALLOCATOR_SCRATCH, (int)(LWS_PRE + len)));
    memcpy(buf + LWS_PRE, text, len);

    lws_write(ws->wsi, buf + LWS_PRE, len, LWS_WRITE_TEXT);
    Free(buf);
}

void PlatformSendBinary(const PlatformWebSocketHandle& handle, const void* data, u32 size) {
    LWSWebSocket* ws = GetSocket(handle);
    if (!ws || !ws->wsi) return;

    u8* buf = (u8*)Alloc(ALLOCATOR_SCRATCH, LWS_PRE + size);
    memcpy(buf + LWS_PRE, data, size);

    lws_write(ws->wsi, buf + LWS_PRE, size, LWS_WRITE_BINARY);
    Free(buf);
}

void PlatformClose(const PlatformWebSocketHandle& handle, u16 code, const char* reason) {
    LWSWebSocket* ws = GetSocket(handle);
    if (!ws) return;

    ws->should_close = true;
    ws->close_code = code;
    if (reason) {
        strncpy(ws->close_reason, reason, sizeof(ws->close_reason) - 1);
    }

    if (ws->wsi) {
        lws_close_reason(ws->wsi, (enum lws_close_status)code, (unsigned char*)reason, reason ? strlen(reason) : 0);
        lws_callback_on_writable(ws->wsi);
    }
}

void PlatformFree(const PlatformWebSocketHandle& handle) {
    LWSWebSocket* ws = GetSocket(handle);
    if (!ws) return;

    // Stop thread
    ws->thread_running = false;
    if (ws->thread_handle) {
#ifdef _WIN32
        WaitForSingleObject(ws->thread_handle, INFINITE);
        CloseHandle(ws->thread_handle);
#else
        pthread_join(*(pthread_t*)ws->thread_handle, nullptr);
        free(ws->thread_handle);
#endif
        ws->thread_handle = nullptr;
    }

    // Cleanup
    if (ws->context) {
        lws_context_destroy(ws->context);
        ws->context = nullptr;
    }

    // Free messages
    for (int i = 0; i < MAX_MESSAGES_PER_SOCKET; i++) {
        if (ws->messages[i].data) {
            Free(ws->messages[i].data);
        }
    }

    if (ws->receive_buffer) {
        Free(ws->receive_buffer);
    }

    DeleteCS(ws->cs);

    ws->status = WebSocketStatus::None;
}

WebSocketStatus PlatformGetStatus(const PlatformWebSocketHandle& handle) {
    LWSWebSocket* ws = GetSocket(handle);
    if (!ws) return WebSocketStatus::None;

    EnterCS(ws->cs);
    WebSocketStatus status = ws->status;
    LeaveCS(ws->cs);

    return status;
}

bool PlatformHasMessages(const PlatformWebSocketHandle& handle) {
    LWSWebSocket* ws = GetSocket(handle);
    if (!ws) return false;

    EnterCS(ws->cs);
    bool has_messages = ws->message_count > 0;
    LeaveCS(ws->cs);

    return has_messages;
}

bool PlatformGetMessage(const PlatformWebSocketHandle& handle, WebSocketMessageType* out_type, u8** out_data, u32* out_size) {
    LWSWebSocket* ws = GetSocket(handle);
    if (!ws) return false;

    EnterCS(ws->cs);

    if (ws->message_count == 0) {
        LeaveCS(ws->cs);
        return false;
    }

    WebSocketMessage& msg = ws->messages[ws->message_read_index];
    if (out_type) *out_type = msg.type;
    if (out_data) *out_data = msg.data;
    if (out_size) *out_size = msg.size;

    LeaveCS(ws->cs);
    return true;
}

void PlatformPopMessage(const PlatformWebSocketHandle& handle) {
    LWSWebSocket* ws = GetSocket(handle);
    if (!ws) return;

    EnterCS(ws->cs);

    if (ws->message_count > 0) {
        WebSocketMessage& msg = ws->messages[ws->message_read_index];
        if (msg.data) {
            Free(msg.data);
            msg.data = nullptr;
        }

        ws->message_read_index = (ws->message_read_index + 1) % MAX_MESSAGES_PER_SOCKET;
        ws->message_count--;
    }

    LeaveCS(ws->cs);
}

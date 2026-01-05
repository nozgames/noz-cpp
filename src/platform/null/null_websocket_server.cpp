//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../../platform.h"

PlatformWebSocketServerHandle PlatformCreateWebSocketServer(const PlatformWebSocketServerConfig& config) {
    (void)config;
    PlatformWebSocketServerHandle handle;
    handle.value = 0;
    return handle;
}

void PlatformDestroyWebSocketServer(const PlatformWebSocketServerHandle& handle) {
    (void)handle;
}

void PlatformUpdateWebSocketServer(const PlatformWebSocketServerHandle& handle) {
    (void)handle;
}

void PlatformWebSocketServerSend(const PlatformWebSocketConnectionHandle& connection, const char* text) {
    (void)connection;
    (void)text;
}

void PlatformWebSocketServerSendBinary(const PlatformWebSocketConnectionHandle& connection, const void* data, u32 size) {
    (void)connection;
    (void)data;
    (void)size;
}

void PlatformWebSocketServerBroadcast(const PlatformWebSocketServerHandle& handle, const char* text) {
    (void)handle;
    (void)text;
}

void PlatformWebSocketServerBroadcastBinary(const PlatformWebSocketServerHandle& handle, const void* data, u32 size) {
    (void)handle;
    (void)data;
    (void)size;
}

void PlatformWebSocketServerClose(const PlatformWebSocketConnectionHandle& connection) {
    (void)connection;
}

u32 PlatformWebSocketServerGetConnectionCount(const PlatformWebSocketServerHandle& handle) {
    (void)handle;
    return 0;
}

bool PlatformWebSocketServerIsRunning(const PlatformWebSocketServerHandle& handle) {
    (void)handle;
    return false;
}

u32 PlatformWebSocketServerGetConnectionId(const PlatformWebSocketConnectionHandle& connection) {
    (void)connection;
    return 0;
}

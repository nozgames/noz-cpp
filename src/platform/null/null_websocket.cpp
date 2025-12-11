//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

void PlatformInitWebSocket() {
}

void PlatformShutdownWebSocket() {
}

void PlatfrormUpdateWebSocket() {
}

PlatformWebSocketHandle PlatformConnectWebSocket(const char* url) {
    (void)url;
    PlatformWebSocketHandle handle;
    handle.value = 0xFFFFFFFFFFFFFFFF;
    return handle;
}

void PlatformSend(const PlatformWebSocketHandle& handle, const char* text) {
    (void)handle;
    (void)text;
}

void PlatformSendBinary(const PlatformWebSocketHandle& handle, const void* data, u32 size) {
    (void)handle;
    (void)data;
    (void)size;
}

void PlatformClose(const PlatformWebSocketHandle& handle, u16 code, const char* reason) {
    (void)handle;
    (void)code;
    (void)reason;
}

void PlatformFree(const PlatformWebSocketHandle& handle) {
    (void)handle;
}

WebSocketStatus PlatformGetStatus(const PlatformWebSocketHandle& handle) {
    (void)handle;
    return WebSocketStatus::Error;
}

bool PlatformHasMessages(const PlatformWebSocketHandle& handle) {
    (void)handle;
    return false;
}

bool PlatformGetMessage(const PlatformWebSocketHandle& handle, WebSocketMessageType* out_type, const u8** out_data, u32* out_size) {
    (void)handle;
    (void)out_type;
    (void)out_data;
    (void)out_size;
    return false;
}

void PlatformPopMessage(const PlatformWebSocketHandle& handle) {
    (void)handle;
}

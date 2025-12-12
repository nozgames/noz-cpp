//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"

void PlatformInitHttp() {
}

void PlatformShutdownHttp() {
}

void PlatformUpdateHttp() {
}

PlatformHttpHandle PlatformGetURL(const char* url) {
    (void)url;
    PlatformHttpHandle handle;
    handle.value = 0xFFFFFFFFFFFFFFFF;
    return handle;
}

PlatformHttpHandle PlatformPostURL(const char* url, const void* body, u32 body_size, const char* content_type, const char* headers, const char* method) {
    (void)url;
    (void)body;
    (void)body_size;
    (void)content_type;
    (void)headers;
    (void)method;
    PlatformHttpHandle handle;
    handle.value = 0xFFFFFFFFFFFFFFFF;
    return handle;
}

HttpStatus PlatformGetStatus(const PlatformHttpHandle& handle) {
    (void)handle;
    return HttpStatus::Error;
}

int PlatformGetStatusCode(const PlatformHttpHandle& handle) {
    (void)handle;
    return 0;
}

const u8* PlatformGetResponse(const PlatformHttpHandle& handle, u32* out_size) {
    (void)handle;
    if (out_size) *out_size = 0;
    return nullptr;
}

char* PlatformGetResponseHeader(const PlatformHttpHandle& handle, const char* name, Allocator* allocator) {
    (void)handle;
    (void)name;
    (void)allocator;
    return nullptr;
}

void PlatformCancel(const PlatformHttpHandle& handle) {
    (void)handle;
}

void PlatformFree(const PlatformHttpHandle& handle) {
    (void)handle;
}

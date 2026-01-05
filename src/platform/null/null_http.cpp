//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../../platform.h"

void PlatformInitHttp(const ApplicationTraits& traits) {
    (void)traits;
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

PlatformHttpStatus PlatformGetStatus(const PlatformHttpHandle& handle) {
    (void)handle;
    return PLATFORM_HTTP_STATUS_ERROR;
}

int PlatformGetStatusCode(const PlatformHttpHandle& handle) {
    (void)handle;
    return 0;
}

bool PlatformIsFromCache(const PlatformHttpHandle& handle) {
    (void)handle;
    return false;
}

Stream* PlatformReleaseResponseStream(const PlatformHttpHandle& handle) {
    (void)handle;
    return nullptr;
}

bool PlatformGetResponseHeader(const PlatformHttpHandle& handle, const char* name, String1024& out) {
    (void)handle;
    (void)name;
    Clear(out);
    return false;
}

void PlatformCancel(const PlatformHttpHandle& handle) {
    (void)handle;
}

void PlatformFree(const PlatformHttpHandle& handle) {
    (void)handle;
}

void PlatformEncodeUrl(char* out, u32 out_size, const char* input, u32 input_length) {
    if (!out || out_size == 0)
        return;

    out[0] = '\0';

    if (!input || input_length == 0)
        return;

    // Null implementation - just copy the input
    u32 copy_len = (input_length < out_size - 1) ? input_length : out_size - 1;
    memcpy(out, input, copy_len);
    out[copy_len] = '\0';
}

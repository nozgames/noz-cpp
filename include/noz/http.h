//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <noz/task.h>
#include <functional>

namespace noz {

    struct HttpRequest {};

    using HttpCallback = std::function<void(Task task, HttpRequest* request)>;

    extern Task GetUrl(const char* url, Task parent = nullptr, const HttpCallback& callback = nullptr);
    extern Task PostUrl(const char* url, const void* body, u32 body_size, const char* content_type = nullptr, const char* headers = nullptr, Task parent = nullptr, const HttpCallback &callback = nullptr);
    extern Task PutUrl(const char* url, const void* body, u32 body_size, const char* content_type = nullptr, const char* headers = nullptr, Task parent = nullptr, const HttpCallback &callback = nullptr);

    extern int GetStatusCode(HttpRequest* request);
    extern bool IsSuccess(HttpRequest* request);
    extern Stream* GetResponseStream(HttpRequest* request);
    extern Stream* ReleaseResponseStream(HttpRequest* request);
    extern bool GetResponseHeader(HttpRequest* request, const char* name, String1024& out);
    extern const char* GetUrl(HttpRequest* request);

    extern void EncodeUrl(Text& out, const Text& input);

}

//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <noz/task.h>
#include <functional>

struct HttpRequest {};

enum class HttpStatus {
    None,
    Pending,
    Complete,
    Error
};

using HttpCallback = std::function<void(HttpRequest* request)>;

// Returns TaskHandle - task result is HttpRequest* when complete
extern noz::TaskHandle GetUrl(const char* url, HttpCallback on_complete = nullptr);
extern noz::TaskHandle PostUrl(const char* url, const void* body, u32 body_size, const char* content_type = nullptr, const char* headers = nullptr, const HttpCallback &on_complete = nullptr);
extern noz::TaskHandle PutUrl(const char* url, const void* body, u32 body_size, const char* content_type = nullptr, const char* headers = nullptr, const HttpCallback &on_complete = nullptr);

// Response accessors - pass the task result (HttpRequest*)
extern int          GetStatusCode(HttpRequest* request);   // HTTP status code (200, 404, etc.)
extern bool         IsSuccess(HttpRequest* request);       // Status 2xx
extern Stream*      GetResponseStream(HttpRequest* request, Allocator* allocator = nullptr);
extern const char*  GetResponseString(HttpRequest* request);  // Null-terminated string (adds \0)
extern u32          GetResponseSize(HttpRequest* request);
extern char*        GetResponseHeader(HttpRequest* request, const char* name, Allocator* allocator);
extern const char*  GetRequestUrl(HttpRequest* request);

extern void InitHttp();
extern void ShutdownHttp();
extern void UpdateHttp();

extern void EncodeUrl(Text& out, const Text& input);

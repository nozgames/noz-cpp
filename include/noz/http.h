//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <functional>

struct HttpRequest;

enum class HttpStatus {
    None,
    Pending,
    Complete,
    Error
};

using HttpCallback = std::function<void(HttpRequest* request)>;

extern HttpRequest* HttpGet(const char* url, HttpCallback on_complete = nullptr);
extern HttpRequest* HttpPost(const char* url, const void* body, u32 body_size, const char* content_type = nullptr, HttpCallback on_complete = nullptr);
extern HttpRequest* HttpPostString(const char* url, const char* body, const char* content_type = "text/plain", HttpCallback on_complete = nullptr);
extern HttpRequest* HttpPostJson(const char* url, const char* json, HttpCallback on_complete = nullptr);

extern HttpStatus   HttpGetStatus(HttpRequest* request);
extern int          HttpGetStatusCode(HttpRequest* request);   // HTTP status code (200, 404, etc.)
extern bool         HttpIsComplete(HttpRequest* request);
extern bool         HttpIsSuccess(HttpRequest* request);       // Complete + status 2xx

extern const u8*    HttpGetResponseData(HttpRequest* request, u32* out_size = nullptr);
extern const char*  HttpGetResponseString(HttpRequest* request);  // Null-terminated string (adds \0)
extern u32          HttpGetResponseSize(HttpRequest* request);

extern void         HttpCancel(HttpRequest* request);
extern void         HttpRelease(HttpRequest* request);

extern void InitHttp();
extern void ShutdownHttp();
extern void UpdateHttp();  // Call each frame to dispatch callbacks

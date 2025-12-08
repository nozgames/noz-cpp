//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <functional>

struct HttpRequest;

enum class HttpStatus
{
    None,
    Pending,
    Complete,
    Error
};

using HttpCallback = std::function<void(HttpRequest* request)>;

// Create and send requests
HttpRequest* HttpGet(const char* url, HttpCallback on_complete = nullptr);
HttpRequest* HttpPost(const char* url, const void* body, u32 body_size, const char* content_type = nullptr, HttpCallback on_complete = nullptr);
HttpRequest* HttpPostString(const char* url, const char* body, const char* content_type = "text/plain", HttpCallback on_complete = nullptr);
HttpRequest* HttpPostJson(const char* url, const char* json, HttpCallback on_complete = nullptr);

// Query request state
HttpStatus   HttpGetStatus(HttpRequest* request);
int          HttpGetStatusCode(HttpRequest* request);   // HTTP status code (200, 404, etc.)
bool         HttpIsComplete(HttpRequest* request);
bool         HttpIsSuccess(HttpRequest* request);       // Complete + status 2xx

// Get response data
const u8*    HttpGetResponseData(HttpRequest* request, u32* out_size = nullptr);
const char*  HttpGetResponseString(HttpRequest* request);  // Null-terminated string (adds \0)
u32          HttpGetResponseSize(HttpRequest* request);

// Cleanup
void         HttpCancel(HttpRequest* request);
void         HttpRelease(HttpRequest* request);

// Module init/shutdown (called by engine)
void InitHttp();
void ShutdownHttp();
void UpdateHttp();  // Call each frame to dispatch callbacks

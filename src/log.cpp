//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include <mutex>
#include <cstdio>
#include <cstdarg>
#include "platform.h"

static LogFunc g_log_callback = nullptr;
static std::mutex g_log_mutex;

void LogImpl(LogType type, const char* format, va_list args) {
    std::lock_guard lock(g_log_mutex);
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer)-1, format, args);
    buffer[sizeof(buffer) - 1] = '\0';

    PlatformLog(type, buffer);

    if (g_log_callback) {
        g_log_callback(type, buffer);
    }
}

void Log(LogType type, const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogImpl(type, format, args);
    va_end(args);
}

void LogInfo(const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogImpl(LOG_TYPE_INFO, format, args);
    va_end(args);
}

void LogWarning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogImpl(LOG_TYPE_WARNING, format, args);
    va_end(args);
}

void LogError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogImpl(LOG_TYPE_ERROR, format, args);
    va_end(args);
}

void LogFileError(const char* filename, const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    
    // Format the message with filename prefix
    int written = snprintf(buffer, sizeof(buffer), "ERROR:%s ", filename);
    vsnprintf(buffer + written, sizeof(buffer) - written, format, args);
    buffer[sizeof(buffer) - 1] = '\0';
    va_end(args);
    
    if (g_log_callback) {
        g_log_callback(LOG_TYPE_ERROR, buffer);
    }
}

void LogShutdown() {
    g_log_callback = nullptr;
}

void InitLog(LogFunc callback) {
    g_log_callback = callback;
}


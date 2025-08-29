//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <cstdio>
#include <cstdarg>
#include <iostream>

static LogFunc g_log_callback = nullptr;

static void LogImpl(LogType type, const char* format, va_list args)
{
    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), format, args);
    buffer[sizeof(buffer) - 1] = '\0';
    
    if (g_log_callback) {
        g_log_callback(type, buffer);
    }
}

void InitLog(LogFunc callback)
{
    g_log_callback = callback;
}

void Log(LogType type, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogImpl(type, format, args);
    va_end(args);
}

void LogInfo(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogImpl(LOG_TYPE_INFO, format, args);
    va_end(args);
}

void LogWarning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogImpl(LOG_TYPE_WARNING, format, args);
    va_end(args);
}

void LogError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogImpl(LOG_TYPE_ERROR, format, args);
    va_end(args);
}

void LogFileError(const char* filename, const char* format, ...)
{
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

void LogShutdown()
{
    g_log_callback = nullptr;
}
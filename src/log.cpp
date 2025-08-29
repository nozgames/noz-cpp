//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/log.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <iostream>

static LogFunc g_log_callback = nullptr;

static void InternalLog(const char* prefix, const char* format, va_list args)
{
    char buffer[2048];
    int written = 0;
    
    if (prefix && strlen(prefix) > 0) {
        written = snprintf(buffer, sizeof(buffer), "[%s] ", prefix);
    }
    
    vsnprintf(buffer + written, sizeof(buffer) - written, format, args);
    buffer[sizeof(buffer) - 1] = '\0';
    if (g_log_callback) {
        g_log_callback(buffer);
    } else {
        printf("%s\n", buffer);
        fflush(stdout);
    }
}

void LogInit(LogFunc callback)
{
    g_log_callback = callback;
}

void Log(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    InternalLog("INFO", format, args);
    va_end(args);
}

void LogWarning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    InternalLog("WARNING", format, args);
    va_end(args);
}

void LogError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    InternalLog("ERROR", format, args);
    va_end(args);
}

void LogFileError(const char* filename, const char* format, ...)
{
    char prefix[256];
    snprintf(prefix, sizeof(prefix), "ERROR:%s", filename);
    
    va_list args;
    va_start(args, format);
    InternalLog(prefix, format, args);
    va_end(args);
}

void LogShutdown()
{
    g_log_callback = nullptr;
}
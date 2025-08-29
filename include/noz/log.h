//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

enum LogType
{
    LOG_TYPE_INFO,
    LOG_TYPE_WARNING,
    LOG_TYPE_ERROR,
    LOG_TYPE_DEBUG
};

typedef void (*LogFunc)(LogType type, const char* message);

void InitLog(LogFunc callback);
void Log(LogType type, const char* format, ...);
void LogInfo(const char* format, ...);
void LogWarning(const char* format, ...);
void LogError(const char* format, ...);
void LogFileError(const char* filename, const char* format, ...);
void LogShutdown();

// Debug logging macro - can be disabled in release builds
#ifndef NDEBUG
    #define LogDebug(...) Log(LOG_TYPE_DEBUG, __VA_ARGS__)
#else
    #define LogDebug(...) ((void)0)
#endif
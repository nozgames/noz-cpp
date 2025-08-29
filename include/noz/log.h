//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef void (*LogFunc)(const char* message);

void LogInit(LogFunc callback);
void Log(const char* format, ...);
void LogWarning(const char* format, ...);
void LogError(const char* format, ...);
void LogFileError(const char* filename, const char* format, ...);
void LogShutdown();
//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

extern void Copy(char* dst, u32 dst_size, const char* src);
extern u32 Length(const char* str);
extern void Format(char* dst, u32 dst_size, const char* fmt, ...);
extern void Format(char* dst, u32 dst_size, const char* fmt, va_list args);

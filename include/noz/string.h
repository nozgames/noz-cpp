//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

extern void Copy(char* dst, u32 dst_size, const char* src);
extern void Copy(char* dst, u32 dst_size, const char* src, u32 length);
extern bool Equals(const char* s1, const char* s2, bool ignore_case = false);
extern bool Equals(const char* s1, const char* s2, u32 length, bool ignore_case = false);
extern int Compare(const char* s1, const char* s2, bool ignore_case = false);
extern u32 Length(const char* str);
extern void Format(char* dst, u32 dst_size, const char* fmt, ...);
extern void Format(char* dst, u32 dst_size, const char* fmt, va_list args);
extern char* CleanPath(char* dst);
extern void Uppercase(char* dst, u32 dst_size);
extern void Lowercase(char* dst, u32 dst_size);
extern void Replace(char* dst, u32 dst_size, char find, char replace);


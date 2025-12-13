//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

extern int Copy(char* dst, int dst_size, const char* src);
extern int Copy(char* dst, int dst_size, const char* src, int length);
extern bool Equals(const char* s1, const char* s2, bool ignore_case = false);
extern bool Equals(const char* s1, const char* s2, u32 length, bool ignore_case = false);
extern bool Contains(const char* s1, const char* s2, bool ignore_case = false);
extern int Compare(const char* s1, const char* s2, bool ignore_case = false);
extern int Length(const char* str);
extern void Format(char* dst, u32 dst_size, const char* fmt, ...);
extern void Format(char* dst, u32 dst_size, const char* fmt, va_list args);
extern char* CleanPath(char* dst);
extern void Uppercase(char* dst, u32 dst_size);
extern void Lowercase(char* dst, u32 dst_size);
extern void Replace(char* dst, u32 dst_size, char find, char replace);

struct String32 {
    char value[32];
    int length;

    operator const char*() const { return value; }
};

struct String128 {
    char value[128];
    int length;

    operator const char*() const { return value; }
};

struct String256 {
    char value[256];
    int length;

    operator const char*() const { return value; }
};

struct String4096 {
    char value[4096];
    int length;

    operator const char*() const { return value; }
};


extern void Set(String32& str, const char* src);
extern void Set(String128& str, const char* src);
extern void Set(String256& str, const char* src);
extern void Set(String4096& str, const char* src);
extern void Set(String32& str, const char* src, u32 length);
extern void Set(String128& str, const char* src, u32 length);
extern void Set(String256& str, const char* src, u32 length);
extern void Set(String4096& str, const char* src, u32 length);
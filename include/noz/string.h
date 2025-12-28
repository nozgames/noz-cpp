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
extern int Format(char* dst, u32 dst_size, const char* fmt, ...);
extern int Format(char* dst, u32 dst_size, const char* fmt, va_list args);
extern char* CleanPath(char* dst);
extern void Upper(char* dst, u32 dst_size);
extern void Lower(char* dst, u32 dst_size);
extern void Replace(char* dst, u32 dst_size, char find, char replace);

struct String32 {
    char value[32];
    int length;

    operator const char*() const { return value; }
};

struct String64 {
    char value[64];
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

struct String1024 {
    char value[1024];
    int length;

    operator const char*() const { return value; }
};

struct String4096 {
    char value[4096];
    int length;

    operator const char*() const { return value; }
};

typedef String4096 Text;

constexpr int TEXT_MAX_LENGTH = 4095;

inline void Set(String32& dst, const char* src) { dst.length = Copy(dst.value, 32, src); }
inline void Set(String64& dst, const char* src) { dst.length = Copy(dst.value, 64, src); }
inline void Set(String128& dst, const char* src) { dst.length = Copy(dst.value, 128, src); }
inline void Set(String256& dst, const char* src) { dst.length = Copy(dst.value, 256, src); }
inline void Set(String1024& dst, const char* src) { dst.length = Copy(dst.value, 1024, src); }
inline void Set(String4096& dst, const char* src) { dst.length = Copy(dst.value, 4096, src); }
inline void Set(String32& dst, const char* src, u32 length) { dst.length = Copy(dst.value, 32, src, length); }
inline void Set(String64& dst, const char* src, u32 length) { dst.length = Copy(dst.value, 64, src, length); }
inline void Set(String128& dst, const char* src, u32 length) { dst.length = Copy(dst.value, 128, src, length); }
inline void Set(String256& dst, const char* src, u32 length) { dst.length = Copy(dst.value, 256, src, length); }
inline void Set(String1024& dst, const char* src, u32 length) { dst.length = Copy(dst.value, 1024, src, length); }
inline void Set(String4096& dst, const char* src, u32 length) { dst.length = Copy(dst.value, 4096, src, length); }

inline void Clear(String32& dst) { dst.length = 0; dst.value[0] = '\0'; }
inline void Clear(String64& dst) { dst.length = 0; dst.value[0] = '\0'; }
inline void Clear(String128& dst) { dst.length = 0; dst.value[0] = '\0'; }
inline void Clear(String256& dst) { dst.length = 0; dst.value[0] = '\0'; }
inline void Clear(String1024& dst) { dst.length = 0; dst.value[0] = '\0'; }
inline void Clear(String4096& dst) { dst.length = 0; dst.value[0] = '\0'; }

inline void Append(String32& dst, const char* src) { dst.length += Copy(dst.value + dst.length, 32 - dst.length, src); }
inline void Append(String64& dst, const char* src) { dst.length += Copy(dst.value + dst.length, 64 - dst.length, src); }
inline void Append(String128& dst, const char* src) { dst.length += Copy(dst.value + dst.length, 128 - dst.length, src); }
inline void Append(String256& dst, const char* src) { dst.length += Copy(dst.value + dst.length, 256 - dst.length, src); }
inline void Append(String1024& dst, const char* src) { dst.length += Copy(dst.value + dst.length, 1024 - dst.length, src); }
inline void Append(String4096& dst, const char* src) { dst.length += Copy(dst.value + dst.length, 4096 - dst.length, src); }
inline void Append(String32& dst, const char* src, u32 length) { dst.length += Copy(dst.value + dst.length, 32 - dst.length, src, length); }
inline void Append(String64& dst, const char* src, u32 length) { dst.length += Copy(dst.value + dst.length, 64 - dst.length, src, length); }
inline void Append(String128& dst, const char* src, u32 length) { dst.length += Copy(dst.value + dst.length, 128 - dst.length, src, length); }
inline void Append(String256& dst, const char* src, u32 length) { dst.length += Copy(dst.value + dst.length, 256 - dst.length, src, length); }
inline void Append(String1024& dst, const char* src, u32 length) { dst.length += Copy(dst.value + dst.length, 1024 - dst.length, src, length); }
inline void Append(String4096& dst, const char* src, u32 length) { dst.length += Copy(dst.value + dst.length, 4096 - dst.length, src, length); }

inline void Upper(String32& dst) { Upper(dst.value, 32); }
inline void Upper(String64& dst) { Upper(dst.value, 64); }
inline void Upper(String128& dst) { Upper(dst.value, 128); }
inline void Upper(String256& dst) { Upper(dst.value, 256); }
inline void Upper(String1024& dst) { Upper(dst.value, 1024); }
inline void Upper(String4096& dst) { Upper(dst.value, 4096); }

inline void Lower(String32& dst) { Lower(dst.value, 32); }
inline void Lower(String64& dst) { Lower(dst.value, 64); }
inline void Lower(String128& dst) { Lower(dst.value, 128); }
inline void Lower(String256& dst) { Lower(dst.value, 256); }
inline void Lower(String1024& dst) { Lower(dst.value, 1024); }
inline void Lower(String4096& dst) { Lower(dst.value, 4096); }

inline bool IsEmpty(const String32& s) { return s.length == 0; }
inline bool IsEmpty(const String64& s) { return s.length == 0; }
inline bool IsEmpty(const String128& s) { return s.length == 0; }
inline bool IsEmpty(const String256& s) { return s.length == 0; }
inline bool IsEmpty(const String1024& s) { return s.length == 0; }
inline bool IsEmpty(const String4096& s) { return s.length == 0; }

extern int Format (String32& dst, const char* fmt, ...);
extern int Format (String64& dst, const char* fmt, ...);
extern int Format (String128& dst, const char* fmt, ...);
extern int Format (String256& dst, const char* fmt, ...);
extern int Format (String1024& dst, const char* fmt, ...);
extern int Format (String4096& dst, const char* fmt, ...);

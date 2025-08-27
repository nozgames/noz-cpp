//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr size_t TEXT_SIZE = 128;

// @text
struct text_t
{
    char value[TEXT_SIZE];
    size_t length;
};

inline void Clear(text_t& text) { text.value[0] = 0; text.length = 0; }
inline void Init(text_t& text) { Clear(text); }
void SetValue(text_t& text, const char* value);
void SetValue(text_t& text, const text_t& value);
void Append(text_t& text, const char* value);
void Append(text_t& text, const text_t& value);
void Format(text_t& text, const char* fmt, ...);
inline size_t GetLength(text_t& text) { return text.length; }
void Trim(text_t& text);
bool Equals(const text_t& a, const char* b);
bool Equals(const text_t& a, const text_t& b);
inline bool IsEmpty(const text_t& text) { return text.length == 0; }
u64 Hash(const text_t& text);

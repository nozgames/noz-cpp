//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr size_t TEXT_MAX_LENGTH = 4095;

// @text
struct Text {
    char value[TEXT_MAX_LENGTH + 1];
    int length;
};

inline void Clear(Text& text) { text.value[0] = 0; text.length = 0; }
inline void Init(Text& text) { Clear(text); }
void SetValue(Text& text, const char* value);
void SetValue(Text& text, const Text& value);
void Append(Text& text, const char* value);
void Append(Text& text, const Text& value);
void Format(Text& text, const char* fmt, ...);
inline size_t GetLength(Text& text) { return text.length; }
void Trim(Text& text);
bool Equals(const Text& a, const char* b);
bool Equals(const Text& a, const Text& b);
inline bool IsEmpty(const Text& text) { return text.length == 0; }
u64 Hash(const Text& text);

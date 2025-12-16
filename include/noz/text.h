//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr int TEXT_MAX_LENGTH = 4095;

// @text
typedef String4096 Text;

inline void Init(Text& text) { Clear(text); }
extern void SetValue(Text& text, const char* value);
extern void SetValue(Text& text, const char* value, int length);
extern void SetValue(Text& text, const Text& value);
extern void Append(Text& text, const char* value);
extern void Append(Text& text, const Text& value);
inline size_t GetLength(Text& text) { return text.length; }
extern void Trim(Text& text);
extern bool Equals(const Text& a, const char* b);
extern bool Equals(const Text& a, const Text& b);
inline bool IsEmpty(const Text& text) { return text.length == 0; }
extern u64 Hash(const Text& text);
inline void Lowercase(Text& text) { Lowercase(text.value, sizeof(text.value)); }

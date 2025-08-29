//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <string>

struct TextInput {};

// Factory function
TextInput* CreateTextInput(int x, int y, int width);
void Destroy(TextInput* input);

// Free functions following NoZ architecture
void Draw(TextInput* input);
void SetActive(TextInput* input, bool active);
bool HandleKey(TextInput* input, int key);
void Clear(TextInput* input);

const std::string& GetText(TextInput* input);
void SetText(TextInput* input, const std::string& text);
size_t GetCursorPos(TextInput* input);
bool IsActive(TextInput* input);
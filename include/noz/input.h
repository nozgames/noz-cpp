//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr int TEXT_INPUT_MAX_LENGTH = 4096;

#include "input_code.h"

// @types
struct InputSet {};

// @input
extern void SetInputSet(InputSet* input_set);
extern InputSet* GetInputSet();
extern void PushInputSet(InputSet* input_set, bool inherit_state=false);
extern void PopInputSet();
extern Vec2 GetMousePosition();
inline bool IsActive(InputSet* input_set) { return GetInputSet() == input_set; }

// @text
struct TextInput
{
    char value[TEXT_INPUT_MAX_LENGTH];
    int length;
    int cursor;
};

extern void BeginTextInput();
extern void EndTextInput();
extern void ClearTextInput();
extern const TextInput& GetTextInput();
extern void SetTextInput(const TextInput& text_input);
extern bool IsTextInputEnabled();

// @InputSet
extern InputSet* CreateInputSet(Allocator* allocator);
extern bool IsButtonDown(InputSet* map, InputCode code);
inline bool IsShiftDown(InputSet* map) { return IsButtonDown(map, KEY_LEFT_SHIFT) || IsButtonDown(map, KEY_RIGHT_SHIFT); }
inline bool IsAltDown(InputSet* map) { return IsButtonDown(map, KEY_LEFT_ALT) || IsButtonDown(map, KEY_RIGHT_ALT); }
inline bool IsCtrlDown(InputSet* map) { return IsButtonDown(map, KEY_LEFT_CTRL) || IsButtonDown(map, KEY_RIGHT_CTRL); }
extern bool WasButtonPressed(InputSet* map, InputCode code);
inline bool WasButtonPressed(InputCode code) { return WasButtonPressed(GetInputSet(), code); }
extern bool WasButtonReleased(InputSet* map, InputCode code);
extern void EnableButton(InputSet* map, InputCode code);
extern void EnableCharacters(InputSet* map);
extern void DisableButton(InputSet* map, InputCode code);
extern float GetAxis(InputSet* set, InputCode code);
extern void ConsumeButton(InputCode code);

extern bool IsButtonDown(InputCode code);
extern bool WasButtonPressed(InputCode code);
extern bool WasButtonReleased(InputCode code);

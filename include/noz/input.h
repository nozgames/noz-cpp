//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "input_code.h"

namespace noz { struct Rect; }

// @types
struct InputSet {};

// @input
extern void SetInputSet(InputSet* input_set);
extern InputSet* GetInputSet();
extern void PushInputSet(InputSet* input_set, bool inherit_state=false);
extern void PopInputSet();
extern Vec2 GetMousePosition();
extern bool IsGamepadActive();
extern bool IsMouseOverWindow();
inline bool IsActive(InputSet* input_set) { return GetInputSet() == input_set; }

// @InputSet
extern InputSet* CreateInputSet(Allocator* allocator, const Name* name=nullptr);
extern bool IsButtonDown(InputSet* map, InputCode code);
extern bool IsButtonDown(InputCode code);
inline bool IsShiftDown() { return IsButtonDown(KEY_LEFT_SHIFT) || IsButtonDown(KEY_RIGHT_SHIFT); }
inline bool IsShiftDown(InputSet* map) { return IsButtonDown(map, KEY_LEFT_SHIFT) || IsButtonDown(map, KEY_RIGHT_SHIFT); }
inline bool IsAltDown(InputSet* map) { return IsButtonDown(map, KEY_LEFT_ALT) || IsButtonDown(map, KEY_RIGHT_ALT); }
inline bool IsCtrlDown(InputSet* map) { return IsButtonDown(map, KEY_LEFT_CTRL) || IsButtonDown(map, KEY_RIGHT_CTRL); }
inline bool IsCtrlDown() { return IsButtonDown(KEY_LEFT_CTRL) || IsButtonDown(KEY_RIGHT_CTRL); }
extern bool WasButtonPressed(InputSet* map, InputCode code);
inline bool WasButtonPressed(InputCode code) { return WasButtonPressed(GetInputSet(), code); }
extern bool WasButtonReleased(InputSet* map, InputCode code);
extern void EnableButton(InputSet* map, InputCode code);
extern void EnableCharacters(InputSet* input_set);
extern void EnableModifiers(InputSet* input_set);
extern void DisableButton(InputSet* map, InputCode code);
extern float GetAxis(InputSet* set, InputCode code);
extern void ConsumeButton(InputCode code);
extern const Name* GetName(InputSet* set);

extern bool IsButtonDown(InputCode code);
extern bool WasButtonPressed(InputCode code);
extern bool WasButtonReleased(InputCode code);

// @native_text_input
// Native text input overlay for web and desktop platforms
// Shows a platform-native text input element positioned over the UI
struct Text;
struct NativeTextboxStyle;
extern void PlatformShowTextbox(const noz::Rect& rect, const Text& text, const NativeTextboxStyle& style);
extern void PlatformHideTextbox();
extern void PlatformUpdateTextboxRect(const noz::Rect& rect, int font_size);
extern bool PlatformUpdateTextboxText(Text& text);
extern bool PlatformIsTextboxVisible();

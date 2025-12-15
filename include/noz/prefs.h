//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

extern void LoadPrefs();
extern void SavePrefs();
extern void SetIntPref(int id, i32 value);
extern void SetIntPref(int id, i32 value);
extern i32 GetIntPref(int id, i32 default_value=0);
extern const char* GetStringPref(int id, const char* default_value=nullptr);
extern void ClearPref(int id);

extern void SetStringPref(int id, const char* value);
inline void SetStringPref(int id, const Text& value) {
    SetStringPref(id, value.value);
}
extern void GetStringPref(int id, Text& out_value);

constexpr int PREF_WINDOW_X = -1;
constexpr int PREF_WINDOW_Y = -2;
constexpr int PREF_WINDOW_WIDTH = -3;
constexpr int PREF_WINDOW_HEIGHT = -4;

constexpr int PREF_CORE_COUNT = 128;


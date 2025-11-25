//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

extern void LoadPrefs();
extern void SavePrefs();
extern void SetPrefInt(int id, i32 value);
extern void SetPrefInt(int id, i32 value);
extern i32 GetPrefInt(int id, i32 default_value=0);
extern void ClearPref(int id);


constexpr int PREF_WINDOW_X = -1;
constexpr int PREF_WINDOW_Y = -2;
constexpr int PREF_WINDOW_WIDTH = -3;
constexpr int PREF_WINDOW_HEIGHT = -4;

constexpr int PREF_CORE_COUNT = 128;


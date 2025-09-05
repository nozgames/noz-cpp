//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

extern void LoadPrefs();
extern void SavePrefs();
extern void SetPrefInt(const Name* name, i32 value);
extern i32 GetPrefInt(const Name* name, i32 default_value=0);

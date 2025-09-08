//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "input_code.h"

// @types
struct InputSet {};

// @input
void SetInputSet(InputSet* map);
InputSet* GetInputSet();
void PushInputSet(InputSet* map);
void PopInputSet();
Vec2 GetMousePosition();

// @InputSet
InputSet* CreateInputSet(Allocator* allocator);
bool IsButtonDown(InputSet* map, InputCode code);
bool WasButtonPressed(InputSet* map, InputCode code);
bool WasButtonReleased(InputSet* map, InputCode code);
void EnableButton(InputSet* map, InputCode code);
void DisableButton(InputSet* map, InputCode code);
float GetAxis(InputSet* set, InputCode code);
void ConsumeButton(InputCode code);

bool IsButtonDown(InputCode code);
bool WasButtonPressed(InputCode code);
bool WasButtonReleased(InputCode code);

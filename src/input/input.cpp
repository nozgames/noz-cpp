//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

void InputActiveInputSetList(LinkedList& list);
void InitInputCodes();
void UpdateInputState(InputSet* input_set);
void ResetInputState(InputSet* input_set);
extern void SetActive(InputSet* input_set, bool active);

struct Input
{
    LinkedList active_sets;
};

static Input g_input = {};

void UpdateInput()
{
    platform::UpdateInputState();
    UpdateInputState((InputSet*)GetBack(g_input.active_sets));
}

void PushInputSet(InputSet* input_set)
{
    assert(input_set);
    assert(!IsInList(g_input.active_sets, input_set));

    SetActive(GetInputSet(), false);
    ResetInputState((InputSet*)GetBack(g_input.active_sets));
    PushBack(g_input.active_sets, input_set);
    SetActive(input_set, true);
}

void PopInputSet()
{
    SetActive(GetInputSet(), false);
    ResetInputState((InputSet*)PopBack(g_input.active_sets));
    SetActive(GetInputSet(), true);
}

InputSet* GetInputSet()
{
    return (InputSet*)GetBack(g_input.active_sets);
}

void SetInputSet(InputSet* input_set)
{
    ResetInputState((InputSet*)GetBack(g_input.active_sets));
    while (GetInputSet())
        PopInputSet();
    PushInputSet(input_set);
}

Vec2 GetMousePosition()
{
    return platform::GetMousePosition();
}

void InitInput()
{
    platform::InitializeInput();
    InitInputCodes();
    InputActiveInputSetList(g_input.active_sets);
}

void ShutdownInput()
{
    platform::ShutdownInput();
}

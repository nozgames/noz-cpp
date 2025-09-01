//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

void InputActiveInputSetList(LinkedList& list);
void InitInputCodes();
void UpdateInputState(InputSet* input_set);
void ResetInputState(InputSet* input_set);

struct Input
{
    vec2 mouse_position;
    LinkedList active_sets;
};

static Input g_input = {};

void UpdateInput()
{
    // Always update the mouse position
    float x;
    float y;
    SDL_GetMouseState(&x, &y);
    g_input.mouse_position = vec2(x, y);

    UpdateInputState((InputSet*)GetBack(g_input.active_sets));
}

void PushInputSet(InputSet* input_set)
{
    assert(input_set);
    assert(!IsInList(g_input.active_sets, input_set));

    ResetInputState((InputSet*)GetBack(g_input.active_sets));
    PushBack(g_input.active_sets, input_set);
}

void PopInputSet()
{
    ResetInputState((InputSet*)PopBack(g_input.active_sets));
}

InputSet* GetInputSet()
{
    return (InputSet*)GetBack(g_input.active_sets);
}

void SetInputSet(InputSet* input_set)
{
    ResetInputState((InputSet*)GetBack(g_input.active_sets));
    Clear(g_input.active_sets);
    PushInputSet(input_set);
}

vec2 GetMousePosition()
{
    return g_input.mouse_position;
}

void InitInput()
{
    InitInputCodes();
    InputActiveInputSetList(g_input.active_sets);
}

void ShutdownInput()
{
}

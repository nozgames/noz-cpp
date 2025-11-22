//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

void InputActiveInputSetList(LinkedList& list);
void UpdateInputState(InputSet* input_set);
void ResetInputState(InputSet* input_set);
extern void SetActive(InputSet* input_set, bool active);
extern void Copy(InputSet* dst, InputSet* src);

struct Input {
    LinkedList active_sets;
    bool text_input;
    bool gamepad_active;
};

static Input g_input = {};

bool IsGamepadActive() {
    return g_input.gamepad_active;
}

void UpdateInput() {
    platform::UpdateInputState();
    UpdateInputState(static_cast<InputSet*>(GetBack(g_input.active_sets)));

    bool gamepad_active = platform::IsGamepadActive();
    if (gamepad_active != g_input.gamepad_active) {
        g_input.gamepad_active = gamepad_active;
        if (gamepad_active) {
            Send(EVENT_GAMEPAD_ACTIVATED, nullptr);
        } else {
            Send(EVENT_GAMEPAD_DEACTIVATED, nullptr);
        }
    }
}

void PushInputSet(InputSet* input_set, bool inherit_state) {
    assert(input_set);
    assert(!IsInList(g_input.active_sets, input_set));

    if (inherit_state)
        Copy(input_set, GetInputSet());
    else
        ResetInputState(input_set);

    ResetInputState(GetInputSet());

    SetActive(GetInputSet(), false);
    PushBack(g_input.active_sets, input_set);
    SetActive(input_set, true);
}

void PopInputSet() {
    SetActive(GetInputSet(), false);
    ResetInputState(static_cast<InputSet*>(PopBack(g_input.active_sets)));
    SetActive(GetInputSet(), true);
    ResetInputState(GetInputSet());
}

InputSet* GetInputSet() {
    return static_cast<InputSet*>(GetBack(g_input.active_sets));
}

void SetInputSet(InputSet* input_set) {
    ResetInputState(static_cast<InputSet*>(GetBack(g_input.active_sets)));
    while (GetInputSet())
        PopInputSet();
    PushInputSet(input_set, false);
}

Vec2 GetMousePosition() {
    return platform::GetMousePosition();
}

void SetTextInput(const TextInput& text_input) {
    platform::SetTextInput(text_input);
}

void ClearTextInput() {
    return platform::ClearTextInput();
}

const TextInput& GetTextInput() {
    return platform::GetTextInput();
}

void BeginTextInput() {
    if (g_input.text_input)
        return;

    g_input.text_input = true;

    ClearTextInput();
}

void EndTextInput() {
    if (!g_input.text_input)
        return;

    ClearTextInput();

    g_input.text_input = false;
}

bool IsTextInputEnabled() {
    return g_input.text_input;
}

void InitInput() {
    assert(INPUT_CODE_COUNT < 256);
    platform::InitializeInput();
    InputActiveInputSetList(g_input.active_sets);
}

void ShutdownInput() {
    platform::ShutdownInput();
}

bool IsButtonDown(InputCode code) {
    return platform::IsInputButtonDown(code);
}

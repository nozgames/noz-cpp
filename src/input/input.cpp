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

bool IsMouseOverWindow() {
    return platform::IsMouseOverWindow();
}

void SetTextInput(const TextInput& text_input) {
    platform::SetTextInput(text_input);
}

void ClearTextInput() {
    return platform::ClearTextInput();
}

void ReplaceSelection(TextInput& input, const char* value) {
    Text& text = input.value;

    // Delete the selection
    if (input.selection_end > input.selection_start) {
        int selection_length = input.selection_end - input.selection_start;
        for (int i = input.selection_start; i < text.length - selection_length; i++)
            text.value[i] = text.value[i + selection_length];
        text.length -= selection_length;
        text.value[text.length] = 0;
        input.cursor = input.selection_start;
        input.selection_start = 0;
        input.selection_end = 0;
    }

    if (!*value)
        return;

    // shift text up to make room for insertion
    int insert_length = Length(value);
    for (int i = 0; i<insert_length; i++)
        text.value[text.length + insert_length - i] = text.value[text.length - i];

    // copy in new text
    for (int i = 0; value[i]; i++)
        text.value[input.cursor + i] = value[i];

    input.selection_start = 0;
    input.selection_end = 0;
    input.value.length += insert_length;
    input.cursor += insert_length;
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

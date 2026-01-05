//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../platform.h"

void InputActiveInputSetList(LinkedList& list);
void UpdateInputState(InputSet* input_set);
void ResetInputState(InputSet* input_set);
extern void SetActive(InputSet* input_set, bool active);
extern void Copy(InputSet* dst, InputSet* src);
extern void InitStrings();

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
    PlatformUpdateInputState();
    UpdateInputState(static_cast<InputSet*>(GetBack(g_input.active_sets)));

    bool gamepad_active = PlatformIsGamepadActive();
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
    Vec2 pos = PlatformGetMousePosition();

    // Transform coordinates when screen is rotated
    if (IsScreenRotated()) {
        // Rotate 90° clockwise: (x, y) -> (height - y, x)
        Vec2Int native_size = PlatformGetWindowSize();
        pos = Vec2{static_cast<f32>(native_size.y) - pos.y, pos.x};
    }

    return pos;
}

bool IsMouseOverWindow() {
    return PlatformIsMouseOverWindow();
}

void InitInput() {
    assert(INPUT_CODE_COUNT < 256);
    PlatformInitInput();
    InputActiveInputSetList(g_input.active_sets);
    InitStrings();
}

void ShutdownInput() {
    PlatformShutdownInput();
}

bool IsButtonDown(InputCode code) {
    return PlatformIsInputButtonDown(code);
}

//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"

constexpr u8 BUTTON_STATE_PRESSED = 1 << 0;
constexpr u8 BUTTON_STATE_RELEASED = 1 << 1;
constexpr u8 BUTTON_STATE_DOWN = 1 << 2;
constexpr u8 BUTTON_STATE_RESET = 1 << 3;
constexpr u8 BUTTON_STATE_ENABLED = 1 << 4;

struct InputSetImpl : InputSet {
    const Name* name;
    u8 buttons[INPUT_CODE_COUNT];
    InputCode enabled_codes[INPUT_CODE_COUNT];
    u32 enabled_count;
    LinkedListNode node_active;
    bool active;
};

const Name* GetName(InputSet* set) {
    return static_cast<InputSetImpl*>(set)->name;
}

void SetActive(InputSet* input_set, bool active)
{
    InputSetImpl* impl = static_cast<InputSetImpl*>(input_set);
    if (!impl)
        return;
    impl->active = active;
}

static bool IsButtonEnabled(u8 state) {
    return (state & BUTTON_STATE_ENABLED) != 0;
}

static bool IsButtonDown(u8 state) {
    return (state & BUTTON_STATE_DOWN) != 0;
}

static bool IsButtonReset(u8 state) {
    return (state & BUTTON_STATE_RESET) != 0;
}

void EnableCharacters(InputSet* input_set) {
    for (int i = KEY_A; i <= KEY_Z; i++)
        EnableButton(input_set, static_cast<InputCode>(i));

    for (int i = KEY_0; i <= KEY_9; i++)
        EnableButton(input_set, static_cast<InputCode>(i));

    EnableButton(input_set, KEY_SPACE);
    EnableButton(input_set, KEY_MINUS);
}

void EnableModifiers(InputSet* input_set) {
    EnableButton(input_set, KEY_LEFT_SHIFT);
    EnableButton(input_set, KEY_RIGHT_SHIFT);
    EnableButton(input_set, KEY_LEFT_CTRL);
    EnableButton(input_set, KEY_RIGHT_CTRL);
    EnableButton(input_set, KEY_LEFT_ALT);
    EnableButton(input_set, KEY_RIGHT_ALT);
}

void EnableButton(InputSet* input_set, InputCode code) {
    InputSetImpl* impl = static_cast<InputSetImpl*>(input_set);
    if (IsButtonEnabled(impl->buttons[code]))
        return;
    impl->enabled_codes[impl->enabled_count++] = code;
    impl->buttons[code] |= BUTTON_STATE_ENABLED;
}

void DisableButton(InputSet* input_set, InputCode code)
{
    InputSetImpl* impl = static_cast<InputSetImpl*>(input_set);
    if (!IsButtonEnabled(impl->buttons[code]))
        return;

    for (u32 i = 0; i < impl->enabled_count; i++)
    {
        if (impl->enabled_codes[i] == code)
        {
            impl->enabled_codes[i] = impl->enabled_codes[impl->enabled_count - 1];
            impl->enabled_count--;
            break;
        }
    }

    impl->buttons[code] = (impl->buttons[code] & ~BUTTON_STATE_ENABLED) | BUTTON_STATE_RESET;
}

void UpdateButtonState(InputSetImpl* impl, InputCode code, bool new_state, bool reset) {
    if (!IsButtonEnabled(impl->buttons[code]))
        return;

    auto old_state = IsButtonDown(impl->buttons[code]);
    if (new_state)
        impl->buttons[code] |= BUTTON_STATE_DOWN;
    else
        impl->buttons[code] &= ~BUTTON_STATE_DOWN;

    if (reset)
        return;

    if (IsButtonReset(impl->buttons[code])) {
        if (!new_state)
            impl->buttons[code] &= (~BUTTON_STATE_RESET);

        return;
    }

    if (new_state != old_state && new_state)
        impl->buttons[code] |= BUTTON_STATE_PRESSED;
    else if (new_state != old_state && !new_state)
        impl->buttons[code] |= BUTTON_STATE_RELEASED;
}

float GetAxis(InputSet* set, InputCode code) {
    if (!((InputSetImpl*)set)->active)
        return 0.0f;
    return platform::GetInputAxisValue(code);
}

void UpdateButtonState(InputSetImpl* impl, bool reset) {
    for (u32 i = 0; i < impl->enabled_count; i++) {
        InputCode code = impl->enabled_codes[i];
        bool button_down = platform::IsInputButtonDown(code);
        UpdateButtonState(impl, code, button_down, reset);
    }
}

void UpdateInputState(InputSet* input_set) {
    if (nullptr == input_set)
        return;

    InputSetImpl* impl = static_cast<InputSetImpl*>(input_set);
    for (int i = 0; i < INPUT_CODE_COUNT; i++)
        impl->buttons[i] &= ~(BUTTON_STATE_PRESSED | BUTTON_STATE_RELEASED);

    UpdateButtonState(impl, false);
}

static bool IsButtonReset(InputSet* input_set, InputCode code) {
    return (static_cast<InputSetImpl*>(input_set)->buttons[code] & BUTTON_STATE_RESET) != 0;
}

bool IsButtonDown(InputSet* input_set, InputCode code) {
    return !IsButtonReset(input_set, code) && (static_cast<InputSetImpl*>(input_set)->buttons[code] & BUTTON_STATE_DOWN) != 0;
}

bool WasButtonPressed(InputSet* input_set, InputCode code) {
    return (static_cast<InputSetImpl*>(input_set)->buttons[code] & BUTTON_STATE_PRESSED) != 0;
}

bool WasButtonReleased(InputSet* input_set, InputCode code) {
    return (static_cast<InputSetImpl*>(input_set)->buttons[code] & BUTTON_STATE_RELEASED) != 0;
}

void Copy(InputSet* dst, InputSet* src) {
    InputSetImpl* dst_impl = static_cast<InputSetImpl*>(dst);
    InputSetImpl* src_impl = static_cast<InputSetImpl*>(src);
    for (u8 i=0; i<INPUT_CODE_COUNT; i++)
        if (IsButtonEnabled(dst_impl->buttons[i]))
            dst_impl->buttons[i] = src_impl->buttons[i] | BUTTON_STATE_ENABLED;
    dst_impl->node_active = {};
}

void ResetInputState(InputSet* input_set) {
    if (!input_set)
        return;

    InputSetImpl* impl = static_cast<InputSetImpl*>(input_set);
    for (int i = 0; i < INPUT_CODE_COUNT; i++)
        impl->buttons[i] = (impl->buttons[i] & BUTTON_STATE_ENABLED) | BUTTON_STATE_RESET;

    UpdateButtonState(impl, true);
}

void InputActiveInputSetList(LinkedList& list) {
    Init(list, offsetof(InputSetImpl, node_active));
}

void ConsumeButton(InputCode code) {
    InputSetImpl* impl = (InputSetImpl*)GetInputSet();
    if (!impl)
        return;

    impl->buttons[code] = (impl->buttons[code] & BUTTON_STATE_ENABLED) | BUTTON_STATE_RESET;
}

InputSet* CreateInputSet(Allocator* allocator, const Name* name) {
    InputSetImpl* impl = (InputSetImpl*)Alloc(allocator, sizeof(InputSetImpl));
    impl->enabled_count = 0;
    impl->name = name == nullptr ? NAME_NONE : name;
    ResetInputState(impl);
    return impl;
}

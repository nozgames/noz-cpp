//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr u8 BUTTON_STATE_PRESSED = 1 << 0;
constexpr u8 BUTTON_STATE_RELEASED = 1 << 1;
constexpr u8 BUTTON_STATE_DOWN = 1 << 2;
constexpr u8 BUTTON_STATE_RESET = 1 << 3;
constexpr u8 BUTTON_STATE_ENABLED = 1 << 4;

struct InputSetImpl : InputSet
{
    u8 buttons[INPUT_CODE_COUNT];
    //SDL_Scancode scancodes[SDL_SCANCODE_COUNT];
    u32 scancode_count;
    LinkedListNode node_active;
};

static bool IsButtonEnabled(u8 state)
{
    return (state & BUTTON_STATE_ENABLED) != 0;
}

static bool IsButtonDown(u8 state)
{
    return (state & BUTTON_STATE_DOWN) != 0;
}

static bool IsButtonReset(u8 state)
{
    return (state & BUTTON_STATE_RESET) != 0;
}

void EnableButton(InputSet* input_set, InputCode code)
{
    InputSetImpl* impl = static_cast<InputSetImpl*>(input_set);
    if (IsButtonEnabled(impl->buttons[code]))
        return;
    //impl->scancodes[impl->scancode_count++] = InputCodeToScanCode(code);
    impl->buttons[code] |= BUTTON_STATE_ENABLED;
}

void DisableButton(InputSet* input_set, InputCode code)
{
    InputSetImpl* impl = static_cast<InputSetImpl*>(input_set);
    if (!IsButtonEnabled(impl->buttons[code]))
        return;

    /*
    auto scancode = InputCodeToScanCode(code);
    for (int i=0; i<impl->scancode_count; i++)
        if (impl->scancodes[i] == scancode)
        {
            impl->scancodes[i] = impl->scancodes[impl->scancode_count - 1];
            impl->scancode_count--;
            break;
        }
        */

    impl->buttons[code] = (impl->buttons[code]&BUTTON_STATE_ENABLED) | BUTTON_STATE_RESET;
}

void UpdateButtonState(InputSetImpl* impl, InputCode code, bool new_state, bool reset)
{
    auto int_code = static_cast<int>(code);
    if (!IsButtonEnabled(impl->buttons[int_code]))
        return;

    auto old_state = IsButtonDown(impl->buttons[int_code]);
    if (new_state)
        impl->buttons[int_code] |= BUTTON_STATE_DOWN;
    else
        impl->buttons[int_code] &= ~BUTTON_STATE_DOWN;

    if (reset)
        return;

    if (IsButtonReset(impl->buttons[int_code]))
    {
        if (!new_state)
            impl->buttons[int_code] &= (~BUTTON_STATE_RESET);

        return;
    }

    if (new_state != old_state && new_state)
        impl->buttons[int_code] |= BUTTON_STATE_PRESSED;
    else if (new_state != old_state && !new_state)
        impl->buttons[int_code] |= BUTTON_STATE_RELEASED;
}

#if 0
void UpdateMouseButtonState(InputSetImpl* impl, SDL_MouseButtonFlags mouse_flags, SDL_MouseButtonFlags mask, InputCode code, bool reset)
{
    if (!IsButtonEnabled(impl->buttons[code]))
        return;

    UpdateButtonState(impl, code, (mouse_flags & mask) != 0, reset);
}
#endif

void UpdateMouseState(InputSetImpl* impl, bool reset)
{
#if 0
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    auto mouse_flags = SDL_GetMouseState(&mouse_x, &mouse_y);
    UpdateMouseButtonState(impl, mouse_flags, SDL_BUTTON_LMASK, MOUSE_LEFT, reset);
    UpdateMouseButtonState(impl, mouse_flags, SDL_BUTTON_RMASK, MOUSE_RIGHT, reset);
    UpdateMouseButtonState(impl, mouse_flags, SDL_BUTTON_MMASK, MOUSE_MIDDLE, reset);
#endif
}

void UpdateKeyboardSate(InputSetImpl* impl, bool reset)
{
#if 0
    const bool* key_states = SDL_GetKeyboardState(nullptr);
    for (u32 i = 0; i < impl->scancode_count; i++)
    {
        auto scancode = impl->scancodes[i];
        auto code = ScanCodeToInputCode(scancode);
        UpdateButtonState(impl, code, key_states[scancode], reset);
    }
#endif
}

void UpdateInputState(InputSet* input_set)
{
    if (nullptr == input_set)
        return;

    InputSetImpl* impl = static_cast<InputSetImpl*>(input_set);
    for (int i = 0; i < INPUT_CODE_COUNT; i++)
        impl->buttons[i] &= ~(BUTTON_STATE_PRESSED | BUTTON_STATE_RELEASED);

    UpdateKeyboardSate(impl, false);
    UpdateMouseState(impl, false);
}

bool IsButtonDown(InputSet* input_set, InputCode code)
{
    return (static_cast<InputSetImpl*>(input_set)->buttons[code] & BUTTON_STATE_DOWN) != 0;
}

bool WasButtonPressed(InputSet* input_set, InputCode code)
{
    return (static_cast<InputSetImpl*>(input_set)->buttons[code] & BUTTON_STATE_PRESSED) != 0;
}

bool WasButtonReleased(InputSet* input_set, InputCode code)
{
    return (static_cast<InputSetImpl*>(input_set)->buttons[code] & BUTTON_STATE_RELEASED) != 0;
}

void ResetInputState(InputSet* input_set)
{
    if (!input_set)
        return;

    InputSetImpl* impl = static_cast<InputSetImpl*>(input_set);
    for (int i = 0; i < INPUT_CODE_COUNT; i++)
        impl->buttons[i] = (impl->buttons[i] & BUTTON_STATE_ENABLED) | BUTTON_STATE_RESET;

    UpdateKeyboardSate(impl, true);
    UpdateMouseState(impl, true);
}

void InputActiveInputSetList(LinkedList& list)
{
    Init(list, offsetof(InputSetImpl, node_active));
}

InputSet* CreateInputSet(Allocator* allocator)
{
    InputSetImpl* impl = (InputSetImpl*)Alloc(allocator, sizeof(InputSetImpl));
    ResetInputState(impl);
    return impl;
}

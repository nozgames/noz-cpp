//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include <noz/input_code.h>
#include <windows.h>
#include <xinput.h>

#pragma comment(lib, "xinput.lib")

extern bool GetWindowFocus();

struct WindowsInput
{
    Vec2 mouse_scroll;
    bool mouse_states[5];
    bool key_states[256];
    XINPUT_STATE gamepad_states[XUSER_MAX_COUNT];
    bool gamepad_connected[XUSER_MAX_COUNT] = {0};
    TextInput text_input;
};

static WindowsInput g_input = {};

static InputCode VKToInputCode(int vk)
{
    switch (vk)
    {
        case 'A': return KEY_A;
        case 'B': return KEY_B;
        case 'C': return KEY_C;
        case 'D': return KEY_D;
        case 'E': return KEY_E;
        case 'F': return KEY_F;
        case 'G': return KEY_G;
        case 'H': return KEY_H;
        case 'I': return KEY_I;
        case 'J': return KEY_J;
        case 'K': return KEY_K;
        case 'L': return KEY_L;
        case 'M': return KEY_M;
        case 'N': return KEY_N;
        case 'O': return KEY_O;
        case 'P': return KEY_P;
        case 'Q': return KEY_Q;
        case 'R': return KEY_R;
        case 'S': return KEY_S;
        case 'T': return KEY_T;
        case 'U': return KEY_U;
        case 'V': return KEY_V;
        case 'W': return KEY_W;
        case 'X': return KEY_X;
        case 'Y': return KEY_Y;
        case 'Z': return KEY_Z;
        case '0': return KEY_0;
        case '1': return KEY_1;
        case '2': return KEY_2;
        case '3': return KEY_3;
        case '4': return KEY_4;
        case '5': return KEY_5;
        case '6': return KEY_6;
        case '7': return KEY_7;
        case '8': return KEY_8;
        case '9': return KEY_9;
        case VK_OEM_PLUS: return KEY_EQUALS;  // = and + key (both handled by same VK)
        case VK_OEM_MINUS: return KEY_MINUS;  // - and _ key
        case VK_OEM_1: return KEY_SEMICOLON;
        case VK_SPACE: return KEY_SPACE;
        case VK_RETURN: return KEY_ENTER;
        case VK_TAB: return KEY_TAB;
        case VK_BACK: return KEY_BACKSPACE;
        case VK_ESCAPE: return KEY_ESCAPE;
        case VK_LSHIFT: return KEY_LEFT_SHIFT;
        case VK_RSHIFT: return KEY_RIGHT_SHIFT;
        case VK_LCONTROL: return KEY_LEFT_CTRL;
        case VK_RCONTROL: return KEY_RIGHT_CTRL;
        case VK_LMENU: return KEY_LEFT_ALT;
        case VK_RMENU: return KEY_RIGHT_ALT;
        case VK_UP: return KEY_UP;
        case VK_DOWN: return KEY_DOWN;
        case VK_LEFT: return KEY_LEFT;
        case VK_RIGHT: return KEY_RIGHT;
        case VK_F1: return KEY_F1;
        case VK_F2: return KEY_F2;
        case VK_F3: return KEY_F3;
        case VK_F4: return KEY_F4;
        case VK_F5: return KEY_F5;
        case VK_F6: return KEY_F6;
        case VK_F7: return KEY_F7;
        case VK_F8: return KEY_F8;
        case VK_F9: return KEY_F9;
        case VK_F10: return KEY_F10;
        case VK_F11: return KEY_F11;
        case VK_F12: return KEY_F12;
        default: return INPUT_CODE_NONE;
    }
}

static float NormalizeStick(SHORT stick_value, SHORT deadzone)
{
    if (stick_value < -deadzone)
        return (stick_value + deadzone) / (32768.0f - deadzone);

    if (stick_value > deadzone)
        return (stick_value - deadzone) / (32767.0f - deadzone);

    return 0.0f;
}

static float NormalizeTrigger(BYTE trigger_value)
{
    if (trigger_value < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        return 0.0f;
    return (trigger_value - XINPUT_GAMEPAD_TRIGGER_THRESHOLD) / (255.0f - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

bool platform::IsInputButtonDown(InputCode code)
{
    // Handle keyboard keys
    if (IsKeyboard(code))
    {
        for (int vk = 0; vk < 256; vk++)
        {
            if (VKToInputCode(vk) == code)
            {
                return g_input.key_states[vk];
            }
        }
        return false;
    }

    // Handle mouse buttons
    if (IsMouse(code) && IsButton(code))
    {
        switch (code)
        {
            case MOUSE_LEFT: return g_input.mouse_states[0];
            case MOUSE_RIGHT: return g_input.mouse_states[1];
            case MOUSE_MIDDLE: return g_input.mouse_states[2];
            case MOUSE_BUTTON_4: return g_input.mouse_states[3];
            case MOUSE_BUTTON_5: return g_input.mouse_states[4];
            default: return false;
        }
    }

    // Handle gamepad buttons
    if (IsGamepad(code) && IsButton(code))
    {
        int gamepad_index = 0; // Default to any gamepad
        WORD button_mask = 0;

        // Determine gamepad index and button mask
        if (code >= GAMEPAD_A && code <= GAMEPAD_RIGHT_TRIGGER)
        {
            // Generic gamepad (use first connected)
            for (int i = 0; i < XUSER_MAX_COUNT; i++)
            {
                if (g_input.gamepad_connected[i])
                {
                    gamepad_index = i;
                    break;
                }
            }
        }
        else if (code >= GAMEPAD_1_A && code <= GAMEPAD_1_RIGHT_TRIGGER)
        {
            gamepad_index = 0;
            code = static_cast<InputCode>(code - GAMEPAD_1_A + GAMEPAD_A);
        }
        else if (code >= GAMEPAD_2_A && code <= GAMEPAD_2_RIGHT_TRIGGER)
        {
            gamepad_index = 1;
            code = static_cast<InputCode>(code - GAMEPAD_2_A + GAMEPAD_A);
        }
        else if (code >= GAMEPAD_3_A && code <= GAMEPAD_3_RIGHT_TRIGGER)
        {
            gamepad_index = 2;
            code = static_cast<InputCode>(code - GAMEPAD_3_A + GAMEPAD_A);
        }
        else if (code >= GAMEPAD_4_A && code <= GAMEPAD_4_RIGHT_TRIGGER)
        {
            gamepad_index = 3;
            code = static_cast<InputCode>(code - GAMEPAD_4_A + GAMEPAD_A);
        }

        if (!g_input.gamepad_connected[gamepad_index])
            return false;

        // Map InputCode to XInput button
        switch (code)
        {
            case GAMEPAD_A: button_mask = XINPUT_GAMEPAD_A; break;
            case GAMEPAD_B: button_mask = XINPUT_GAMEPAD_B; break;
            case GAMEPAD_X: button_mask = XINPUT_GAMEPAD_X; break;
            case GAMEPAD_Y: button_mask = XINPUT_GAMEPAD_Y; break;
            case GAMEPAD_LEFT_SHOULDER: button_mask = XINPUT_GAMEPAD_LEFT_SHOULDER; break;
            case GAMEPAD_RIGHT_SHOULDER: button_mask = XINPUT_GAMEPAD_RIGHT_SHOULDER; break;
            case GAMEPAD_START: button_mask = XINPUT_GAMEPAD_START; break;
            case GAMEPAD_BACK: button_mask = XINPUT_GAMEPAD_BACK; break;
            case GAMEPAD_LEFT_STICK_BUTTON: button_mask = XINPUT_GAMEPAD_LEFT_THUMB; break;
            case GAMEPAD_RIGHT_STICK_BUTTON: button_mask = XINPUT_GAMEPAD_RIGHT_THUMB; break;
            case GAMEPAD_DPAD_UP: button_mask = XINPUT_GAMEPAD_DPAD_UP; break;
            case GAMEPAD_DPAD_DOWN: button_mask = XINPUT_GAMEPAD_DPAD_DOWN; break;
            case GAMEPAD_DPAD_LEFT: button_mask = XINPUT_GAMEPAD_DPAD_LEFT; break;
            case GAMEPAD_DPAD_RIGHT: button_mask = XINPUT_GAMEPAD_DPAD_RIGHT; break;
            case GAMEPAD_LEFT_TRIGGER_BUTTON:
                return g_input.gamepad_states[gamepad_index].Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            case GAMEPAD_RIGHT_TRIGGER_BUTTON:
                return g_input.gamepad_states[gamepad_index].Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            default: return false;
        }

        return (g_input.gamepad_states[gamepad_index].Gamepad.wButtons & button_mask) != 0;
    }

    return false;
}

float platform::GetInputAxisValue(InputCode code)
{
    // Handle mouse axes
    if (IsMouse(code) && IsAxis(code))
    {
        switch (code)
        {
            case MOUSE_X: return GetMousePosition().x;
            case MOUSE_Y: return GetMousePosition().y;
            case MOUSE_SCROLL_X: return GetMouseScroll().x;
            case MOUSE_SCROLL_Y: return GetMouseScroll().y;
            default: return 0.0f;
        }
    }

    // Handle gamepad axes
    if (IsGamepad(code) && IsAxis(code))
    {
        int gamepad_index = 0; // Default to any gamepad

        // Determine gamepad index
        if (code >= GAMEPAD_LEFT_STICK_X && code <= GAMEPAD_RIGHT_TRIGGER)
        {
            // Generic gamepad (use first connected)
            for (int i = 0; i < XUSER_MAX_COUNT; i++)
            {
                if (g_input.gamepad_connected[i])
                {
                    gamepad_index = i;
                    break;
                }
            }
        }
        else if (code >= GAMEPAD_1_LEFT_STICK_X && code <= GAMEPAD_1_RIGHT_TRIGGER)
        {
            gamepad_index = 0;
            code = static_cast<InputCode>(code - GAMEPAD_1_LEFT_STICK_X + GAMEPAD_LEFT_STICK_X);
        }
        else if (code >= GAMEPAD_2_LEFT_STICK_X && code <= GAMEPAD_2_RIGHT_TRIGGER)
        {
            gamepad_index = 1;
            code = static_cast<InputCode>(code - GAMEPAD_2_LEFT_STICK_X + GAMEPAD_LEFT_STICK_X);
        }
        else if (code >= GAMEPAD_3_LEFT_STICK_X && code <= GAMEPAD_3_RIGHT_TRIGGER)
        {
            gamepad_index = 2;
            code = static_cast<InputCode>(code - GAMEPAD_3_LEFT_STICK_X + GAMEPAD_LEFT_STICK_X);
        }
        else if (code >= GAMEPAD_4_LEFT_STICK_X && code <= GAMEPAD_4_RIGHT_TRIGGER)
        {
            gamepad_index = 3;
            code = static_cast<InputCode>(code - GAMEPAD_4_LEFT_STICK_X + GAMEPAD_LEFT_STICK_X);
        }

        if (!g_input.gamepad_connected[gamepad_index])
            return 0.0f;

        const XINPUT_GAMEPAD& gamepad = g_input.gamepad_states[gamepad_index].Gamepad;

        switch (code)
        {
            case GAMEPAD_LEFT_STICK_X:
                return NormalizeStick(gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            case GAMEPAD_LEFT_STICK_Y:
                return NormalizeStick(gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            case GAMEPAD_RIGHT_STICK_X:
                return NormalizeStick(gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            case GAMEPAD_RIGHT_STICK_Y:
                return NormalizeStick(gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            case GAMEPAD_LEFT_TRIGGER:
                return NormalizeTrigger(gamepad.bLeftTrigger);
            case GAMEPAD_RIGHT_TRIGGER:
                return NormalizeTrigger(gamepad.bRightTrigger);
            default:
                return 0.0f;
        }
    }

    return 0.0f;
}


void platform::UpdateInputState()
{
    // todo: limit to actual input set for performance.
    
    if (!HasFocus())
    {
        // Clear all input states when window doesn't have focus
        for (int vk = 0; vk < 256; vk++)
            g_input.key_states[vk] = false;
        
        for (int i = 0; i < 5; i++)
            g_input.mouse_states[i] = false;
        
        g_input.mouse_scroll = {0, 0};
        
        for (int i = 0; i < XUSER_MAX_COUNT; i++)
        {
            ZeroMemory(&g_input.gamepad_states[i], sizeof(XINPUT_STATE));
        }
        
        return;
    }

    // Update keyboard state
    for (int vk = 0; vk < 256; vk++)
        g_input.key_states[vk] = (GetAsyncKeyState(vk) & 0x8000) != 0;

    // Update mouse state
    g_input.mouse_states[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; // Left mouse
    g_input.mouse_states[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0; // Right mouse
    g_input.mouse_states[2] = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0; // Middle mouse
    g_input.mouse_states[3] = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0; // Mouse button 4
    g_input.mouse_states[4] = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0; // Mouse button 5

    // Update gamepad states
    for (int i = 0; i < XUSER_MAX_COUNT; i++)
    {
        XINPUT_STATE state;
        DWORD result = XInputGetState(i, &state);
            
        if (result == ERROR_SUCCESS)
        {
            g_input.gamepad_connected[i] = true;
            g_input.gamepad_states[i] = state;
        }
        else
        {
            g_input.gamepad_connected[i] = false;
        }
    }
}

void HandleInputCharacter(char c)
{
    if (g_input.text_input.length >= TEXT_INPUT_MAX_LENGTH - 1)
        return;

    if (c < 32)
        return;

    g_input.text_input.value[g_input.text_input.length++] = c;
    g_input.text_input.value[g_input.text_input.length] = 0;
    g_input.text_input.cursor++;

    Send(EVENT_TEXTINPUT_CHANGED, &g_input.text_input);
}

void HandleInputKeyDown(char c)
{
    switch (c)
    {
    case VK_BACK:
        if (g_input.text_input.length > 0 && g_input.text_input.cursor > 0)
        {
            // Move characters back one position from cursor
            for (int i = g_input.text_input.cursor - 1; i < g_input.text_input.length - 1; i++)
                g_input.text_input.value[i] = g_input.text_input.value[i + 1];
            g_input.text_input.length--;
            g_input.text_input.value[g_input.text_input.length] = 0;
            g_input.text_input.cursor--;
            Send(EVENT_TEXTINPUT_CHANGED, &g_input.text_input);
        }
        break;
    }
}

void platform::ClearTextInput()
{
    if (g_input.text_input.length == 0)
        return;

    g_input.text_input.value[0] = 0;
    g_input.text_input.length = 0;
    g_input.text_input.cursor = 0;
    Send(EVENT_TEXTINPUT_CHANGED, &g_input.text_input);
}

const TextInput& platform::GetTextInput()
{
    return g_input.text_input;
}

void platform::SetTextInput(const TextInput& text_input)
{
    g_input.text_input = text_input;
}

void platform::InitializeInput()
{
    ClearTextInput();

    for (int i = 0; i < XUSER_MAX_COUNT; i++)
    {
        ZeroMemory(&g_input.gamepad_states[i], sizeof(XINPUT_STATE));
        g_input.gamepad_connected[i] = false;
    }
}

void platform::ShutdownInput()
{
    // Nothing to cleanup for XInput
}

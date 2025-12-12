//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include <noz/input_code.h>
#include <windows.h>
#include <xinput.h>

#pragma comment(lib, "xinput.lib")

extern bool GetWindowFocus();

struct WindowsInput {
    Vec2 mouse_scroll;
    bool key_states[INPUT_CODE_COUNT];
    int virtual_keys[INPUT_CODE_COUNT];
    XINPUT_STATE gamepad_states[XUSER_MAX_COUNT];
    bool gamepad_connected[XUSER_MAX_COUNT] = {0};
    Vec2 gamepad_left_stick[XUSER_MAX_COUNT] = {0};
    TextInput text_input;
    int active_controller;
    HWND edit_hwnd = nullptr;
    WNDPROC edit_proc = nullptr;
    Text edit_text;
    bool edit_visible = false;
    HBRUSH edit_bg_brush = nullptr;
    HFONT edit_font = nullptr;
    COLORREF edit_text_color = RGB(255, 255, 255);
    COLORREF edit_bg_color = RGB(55, 55, 55);
};

static WindowsInput g_windows_input = {};

static InputCode VKToInputCode(int vk) {
    switch (vk) {
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
        case VK_OEM_PLUS: return KEY_EQUALS;
        case VK_OEM_MINUS: return KEY_MINUS;
        case VK_SPACE: return KEY_SPACE;
        case VK_RETURN: return KEY_ENTER;
        case VK_TAB: return KEY_TAB;
        case VK_OEM_COMMA: return KEY_COMMA;
        case VK_OEM_PERIOD: return KEY_PERIOD;
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
        case VK_OEM_1: return KEY_SEMICOLON;
        case VK_OEM_3: return KEY_TILDE;
        case VK_OEM_4: return KEY_LEFT_BRACKET;
        case VK_OEM_6: return KEY_RIGHT_BRACKET;
        case VK_OEM_7: return KEY_QUOTE;
        case VK_LBUTTON: return MOUSE_LEFT;
        case VK_RBUTTON: return MOUSE_RIGHT;
        case VK_MBUTTON: return MOUSE_MIDDLE;
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

static InputCode GetLeftStickButton(int gamepad_index) {
    const Vec2& stick = g_windows_input.gamepad_left_stick[gamepad_index];
    if (stick.y > 0.5f)
        return GAMEPAD_LEFT_STICK_UP;
    if (stick.y < -0.5f)
        return GAMEPAD_LEFT_STICK_DOWN;
    if (stick.x < -0.5f)
        return GAMEPAD_LEFT_STICK_LEFT;
    if (stick.x > 0.5f)
        return GAMEPAD_LEFT_STICK_RIGHT;

    return INPUT_CODE_NONE;
}

bool PlatformIsInputButtonDown(InputCode code) {
    if (IsMouse(code) || IsKeyboard(code))
        return g_windows_input.key_states[code];

    // Handle gamepad buttons
    if (IsGamepad(code) && IsButton(code)) {
        int gamepad_index = 0; // Default to any gamepad
        WORD button_mask = 0;

        // Determine gamepad index and button mask
        if (code >= GAMEPAD_A && code <= GAMEPAD_RIGHT_TRIGGER) {
            for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                if (g_windows_input.gamepad_connected[i]) {
                    gamepad_index = i;
                    break;
                }
            }
        } else if (code >= GAMEPAD_1_A && code <= GAMEPAD_1_RIGHT_TRIGGER_BUTTON) {
            gamepad_index = 0;
            code = static_cast<InputCode>(code - GAMEPAD_1_A + GAMEPAD_A);
        } else if (code >= GAMEPAD_2_A && code <= GAMEPAD_2_RIGHT_TRIGGER) {
            gamepad_index = 1;
            code = static_cast<InputCode>(code - GAMEPAD_2_A + GAMEPAD_A);
        } else if (code >= GAMEPAD_3_A && code <= GAMEPAD_3_RIGHT_TRIGGER) {
            gamepad_index = 2;
            code = static_cast<InputCode>(code - GAMEPAD_3_A + GAMEPAD_A);
        } else if (code >= GAMEPAD_4_A && code <= GAMEPAD_4_RIGHT_TRIGGER) {
            gamepad_index = 3;
            code = static_cast<InputCode>(code - GAMEPAD_4_A + GAMEPAD_A);
        }

        if (!g_windows_input.gamepad_connected[gamepad_index])
            return false;

        // Map InputCode to XInput button
        switch (code) {
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
                return g_windows_input.gamepad_states[gamepad_index].Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            case GAMEPAD_RIGHT_TRIGGER_BUTTON:
                return g_windows_input.gamepad_states[gamepad_index].Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
            case GAMEPAD_LEFT_STICK_LEFT:
            case GAMEPAD_LEFT_STICK_RIGHT:
            case GAMEPAD_LEFT_STICK_UP:
            case GAMEPAD_LEFT_STICK_DOWN:
                return GetLeftStickButton(gamepad_index) == code;
                break;
            default: return false;
        }

        return (g_windows_input.gamepad_states[gamepad_index].Gamepad.wButtons & button_mask) != 0;
    }

    return false;
}

static bool WasGamepadUsed(int gamepad_index) {
    const XINPUT_GAMEPAD& gamepad = g_windows_input.gamepad_states[gamepad_index].Gamepad;

    if (NormalizeStick(gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) > F32_EPSILON)
        return true;
    if (NormalizeStick(gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) > F32_EPSILON)
        return true;
    if (NormalizeStick(gamepad.sThumbRX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) > F32_EPSILON)
        return true;
    if (NormalizeStick(gamepad.sThumbRY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) > F32_EPSILON)
        return true;
    if (g_windows_input.gamepad_states[gamepad_index].Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        return true;
    if (g_windows_input.gamepad_states[gamepad_index].Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        return true;

    return false;
}

float PlatformGetInputAxisValue(InputCode code) {
    // Handle mouse axes
    if (IsMouse(code) && IsAxis(code)) {
        switch (code)
        {
            case MOUSE_X: return PlatformGetMousePosition().x;
            case MOUSE_Y: return PlatformGetMousePosition().y;
            case MOUSE_SCROLL_X: return PlatformGetMouseScroll().x;
            case MOUSE_SCROLL_Y: return PlatformGetMouseScroll().y;
            default: return 0.0f;
        }
    }

    // Handle gamepad axes
    if (IsGamepad(code) && IsAxis(code)) {
        int gamepad_index = 0; // Default to any gamepad

        // Determine gamepad index
        if (code >= GAMEPAD_LEFT_STICK_X && code <= GAMEPAD_RIGHT_TRIGGER)
        {
            // Generic gamepad (use first connected)
            for (int i = 0; i < XUSER_MAX_COUNT; i++)
            {
                if (g_windows_input.gamepad_connected[i])
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

        if (!g_windows_input.gamepad_connected[gamepad_index])
            return 0.0f;

        const XINPUT_GAMEPAD& gamepad = g_windows_input.gamepad_states[gamepad_index].Gamepad;

        switch (code) {
            case GAMEPAD_LEFT_STICK_X:
                return NormalizeStick(gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            case GAMEPAD_LEFT_STICK_Y:
                return -NormalizeStick(gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            case GAMEPAD_RIGHT_STICK_X:
                return NormalizeStick(gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
            case GAMEPAD_RIGHT_STICK_Y:
                return -NormalizeStick(gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
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


void PlatformUpdateInputState() {
    if (!PlatformIsWindowFocused()) {
        for (int i = 0; i < INPUT_CODE_COUNT; i++)
            g_windows_input.key_states[i] = false;

        g_windows_input.mouse_scroll = {0, 0};

        for (int i = 0; i < XUSER_MAX_COUNT; i++)
            ZeroMemory(&g_windows_input.gamepad_states[i], sizeof(XINPUT_STATE));

        return;
    }

    bool key_state_changed = false;
    for (int i = 0; i < INPUT_CODE_COUNT; i++) {
        bool old_state = g_windows_input.key_states[i];
        g_windows_input.key_states[i] = GetAsyncKeyState(g_windows_input.virtual_keys[i]) < 0;
        key_state_changed |= (old_state != g_windows_input.key_states[i]);
    }

    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        XINPUT_STATE state;
        DWORD result = XInputGetState(i, &state);

        if (result == ERROR_SUCCESS) {
            g_windows_input.gamepad_connected[i] = true;
            g_windows_input.gamepad_states[i] = state;

            g_windows_input.gamepad_left_stick[i] = Vec2{
                NormalizeStick(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE),
                NormalizeStick(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            };

            if (g_windows_input.active_controller != i && WasGamepadUsed(i)) {
                g_windows_input.active_controller = i;
            }

        } else {
            g_windows_input.gamepad_connected[i] = false;
            if (g_windows_input.active_controller == i) {
                g_windows_input.active_controller = -1;
            }
        }
    }

    if (key_state_changed) {
        g_windows_input.active_controller = -1;
    }
}

void HandleInputCharacter(char c) {
    if (!IsTextInputEnabled())
        return;

    if (g_windows_input.text_input.value.length >= TEXT_MAX_LENGTH)
        return;

    if (c == 27) {
        Send(EVENT_TEXTINPUT_CANCEL, &g_windows_input.text_input);
        return;
    }

    if (c == 13) {
        Send(EVENT_TEXTINPUT_COMMIT, &g_windows_input.text_input);
        return;
    }

    if (c < 32)
        return;

    char text[] = { c, 0 };
    ReplaceSelection(g_windows_input.text_input, text);

    Send(EVENT_TEXTINPUT_CHANGE, &g_windows_input.text_input);
}

void HandleInputKeyDown(char c) {
    if (!IsTextInputEnabled())
        return;

    TextInput& input = g_windows_input.text_input;
    if (c == VK_BACK) {
        if (input.selection_end > input.selection_start) {
            ReplaceSelection(input, "");
            return;
        }

        if (input.cursor < 1)
            return;

        input.selection_start = input.cursor - 1;
        input.selection_end = input.cursor;
        ReplaceSelection(input, "");

        Send(EVENT_TEXTINPUT_CHANGE, &g_windows_input.text_input);
        return;
    } else if (c == VK_DELETE) {
        if (input.selection_end > input.selection_start) {
            ReplaceSelection(input, "");
            return;
        }

        if (input.cursor >= input.value.length)
            return;

        input.selection_start = input.cursor;
        input.selection_end = input.cursor + 1;
        ReplaceSelection(input, "");

        Send(EVENT_TEXTINPUT_CHANGE, &g_windows_input.text_input);
        return;

    } if (c == 36) {
        if (IsShiftDown()) {
            input.selection_start = 0;
            input.selection_end = input.cursor;
        } else {
            input.cursor = 0;
            input.selection_start = 0;
            input.selection_end = 0;
        }
    } if (c == 35) {
        if (IsShiftDown()) {
            input.selection_start = input.cursor;
            input.selection_end = input.value.length;
        } else {
            input.cursor = input.value.length;
            input.selection_start = 0;
            input.selection_end = 0;
        }
    } else if (c == 37) {
        if (input.cursor > 0) {
            input.cursor--;
            if (IsShiftDown()) {
                if (input.selection_start == input.selection_end) {
                    input.selection_start = input.cursor;
                    input.selection_end = input.cursor + 1;
                } else if (input.cursor < input.selection_start) {
                    input.selection_start = input.cursor;
                } else {
                    input.selection_end = input.cursor;
                }
            } else {
                input.selection_start = 0;
                input.selection_end = 0;
            }
        }
    } else if (c == 39) {
        if (input.cursor < input.value.length) {
            input.cursor++;
            if (IsShiftDown()) {
                if (input.selection_start == input.selection_end) {
                    input.selection_start = input.cursor - 1;
                    input.selection_end = input.cursor;
                } else if (input.cursor > input.selection_end) {
                    input.selection_end = input.cursor;
                } else {
                    input.selection_start = input.cursor;
                }
            } else {
                input.selection_start = 0;
                input.selection_end = 0;
            }
        }
    }
}

void PlatformClearTextInput() {
    Text& text = g_windows_input.text_input.value;
    if (text.length == 0)
        return;

    text.value[0] = 0;
    text.length = 0;
    g_windows_input.text_input.cursor = 0;
    Send(EVENT_TEXTINPUT_CHANGE, &g_windows_input.text_input);
}

const TextInput& PlatformGetTextInput()
{
    return g_windows_input.text_input;
}

void PlatformSetTextInput(const TextInput& text_input)
{
    g_windows_input.text_input = text_input;
}

bool PlatformIsGamepadActive() {
    return g_windows_input.active_controller != -1;
}

extern HWND PlatformGetWindowHandle();

HBRUSH GetNativeEditBrush() {
    return g_windows_input.edit_bg_brush;
}

COLORREF GetNativeEditTextColor() {
    return g_windows_input.edit_text_color;
}

COLORREF GetNativeEditBgColor() {
    return g_windows_input.edit_bg_color;
}

static void CommitNativeEdit(HWND hwnd) {
    int len = GetWindowTextA(hwnd, g_windows_input.edit_text.value, TEXT_MAX_LENGTH);
    g_windows_input.edit_text.length = len;
    ShowWindow(hwnd, SW_HIDE);
    g_windows_input.edit_visible = false;
    HWND parent = PlatformGetWindowHandle();
    if (parent) SetFocus(parent);
}

static LRESULT CALLBACK NativeEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CHAR:
            // Suppress beep for Enter and Escape
            if (wParam == '\r' || wParam == 27)
                return 0;
            break;
        case WM_KEYDOWN:
            if (wParam == VK_RETURN) {
                CommitNativeEdit(hwnd);
                return 0;
            } else if (wParam == VK_ESCAPE) {
                // Cancel and hide (don't update text value)
                ShowWindow(hwnd, SW_HIDE);
                g_windows_input.edit_visible = false;
                HWND parent = PlatformGetWindowHandle();
                if (parent) SetFocus(parent);
                return 0;
            }
            break;
        case WM_KILLFOCUS:
            if (g_windows_input.edit_visible) {
                CommitNativeEdit(hwnd);
            }
            break;
    }
    return CallWindowProc(g_windows_input.edit_proc, hwnd, msg, wParam, lParam);
}

static COLORREF ColorToColorRef(const Color& c) {
    return RGB(
        static_cast<BYTE>(c.r * 255.0f),
        static_cast<BYTE>(c.g * 255.0f),
        static_cast<BYTE>(c.b * 255.0f)
    );
}

static void UpdateEditFont(int font_size) {
    if (g_windows_input.edit_font) {
        DeleteObject(g_windows_input.edit_font);
    }
    g_windows_input.edit_font = CreateFontA(
        -font_size,
        0,
        0,
        0,
        FW_SEMIBOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        "Segoe UI"
    );
    SendMessage(g_windows_input.edit_hwnd, WM_SETFONT, (WPARAM)g_windows_input.edit_font, TRUE);
}

static void PositionEditCentered(int x, int y, int width, int height, UINT flags) {
    HDC hdc = GetDC(g_windows_input.edit_hwnd);
    HFONT old_font = (HFONT)SelectObject(hdc, g_windows_input.edit_font);
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, old_font);
    ReleaseDC(g_windows_input.edit_hwnd, hdc);

    int font_height = tm.tmHeight;
    int y_offset = (height - font_height) / 2;
    SetWindowPos(g_windows_input.edit_hwnd, HWND_TOP, x, y + y_offset, width, font_height, flags);
    SendMessage(g_windows_input.edit_hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
}

void PlatformShowNativeTextInput(const noz::Rect& screen_rect, const char* initial_value, const NativeTextInputStyle& style) {
    HWND parent = PlatformGetWindowHandle();
    if (!parent) return;

    int x = static_cast<int>(screen_rect.x);
    int y = static_cast<int>(screen_rect.y);
    int width = static_cast<int>(screen_rect.width);
    int height = static_cast<int>(screen_rect.height);

    // Update colors
    g_windows_input.edit_text_color = ColorToColorRef(style.text_color);
    g_windows_input.edit_bg_color = ColorToColorRef(style.background_color);
    if (g_windows_input.edit_bg_brush) {
        DeleteObject(g_windows_input.edit_bg_brush);
    }
    g_windows_input.edit_bg_brush = CreateSolidBrush(g_windows_input.edit_bg_color);

    // Update existing edit control
    if (g_windows_input.edit_hwnd && g_windows_input.edit_visible && initial_value == nullptr) {
        UpdateEditFont(style.font_size);
        PositionEditCentered(x, y, width, height, SWP_NOACTIVATE);
        InvalidateRect(g_windows_input.edit_hwnd, nullptr, TRUE);
        return;
    }

    // Create edit control if needed
    if (!g_windows_input.edit_hwnd) {
        g_windows_input.edit_hwnd = CreateWindowExA(
            0, "EDIT", initial_value ? initial_value : "",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT,
            x, y, width, height,
            parent, nullptr, GetModuleHandle(nullptr), nullptr
        );
        if (g_windows_input.edit_hwnd) {
            g_windows_input.edit_proc = (WNDPROC)SetWindowLongPtr(
                g_windows_input.edit_hwnd, GWLP_WNDPROC, (LONG_PTR)NativeEditProc);
        }
    } else {
        SetWindowPos(g_windows_input.edit_hwnd, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
        if (initial_value) {
            SetWindowTextA(g_windows_input.edit_hwnd, initial_value);
        }
    }

    if (g_windows_input.edit_hwnd) {
        UpdateEditFont(style.font_size);
        PositionEditCentered(x, y, width, height, SWP_SHOWWINDOW);
        SetFocus(g_windows_input.edit_hwnd);

        if (initial_value) {
            SendMessage(g_windows_input.edit_hwnd, EM_SETSEL, 0, -1);
            strncpy(g_windows_input.edit_text.value, initial_value, TEXT_MAX_LENGTH);
            g_windows_input.edit_text.value[TEXT_MAX_LENGTH] = 0;
        }

        g_windows_input.edit_visible = true;
    }
}

void PlatformHideNativeTextInput() {
    if (!g_windows_input.edit_hwnd || !g_windows_input.edit_visible)
        return;

    ShowWindow(g_windows_input.edit_hwnd, SW_HIDE);

    HWND parent = PlatformGetWindowHandle();
    if (parent) SetFocus(parent);
    g_windows_input.edit_visible = false;
}

bool PlatformIsNativeTextInputVisible() {
    return g_windows_input.edit_visible;
}

const char* PlatformGetNativeTextInputValue() {
    return g_windows_input.edit_text.value;
}

void PlatformInitInput()
{
    PlatformClearTextInput();

    for (int i = 0; i < XUSER_MAX_COUNT; i++) {
        ZeroMemory(&g_windows_input.gamepad_states[i], sizeof(XINPUT_STATE));
        g_windows_input.gamepad_connected[i] = false;
    }

    for (int vk=0; vk<256; vk++) {
        InputCode code = VKToInputCode(vk);
        if (code == INPUT_CODE_NONE)
            continue;
        g_windows_input.virtual_keys[code] = vk;
    }
}

void PlatformShutdownInput() {
}

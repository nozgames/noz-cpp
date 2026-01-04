//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include <noz/input_code.h>
#include <windows.h>
#include <xinput.h>

#pragma comment(lib, "xinput.lib")

extern bool GetWindowFocus();
extern HWND GetWindowHandle();
extern bool PlatformIsMouseOverWindow();

struct WindowsInput {
    Vec2 mouse_scroll;
    bool key_states[INPUT_CODE_COUNT];
    int virtual_keys[INPUT_CODE_COUNT];
    XINPUT_STATE gamepad_states[XUSER_MAX_COUNT];
    bool gamepad_connected[XUSER_MAX_COUNT] = {0};
    Vec2 gamepad_left_stick[XUSER_MAX_COUNT] = {0};
    int active_controller;
    bool double_click;

    HWND edit_hwnd = nullptr;
    WNDPROC edit_proc = nullptr;
    bool edit_visible = false;
    bool edit_dirty = false;
    noz::RectInt edit_rect;
    COLORREF edit_text_color = RGB(255, 255, 255);
    COLORREF edit_bg_color = RGB(55, 55, 55);
    HBRUSH edit_bg_brush = nullptr;
    HFONT edit_font = nullptr;
    int edit_font_size = -1;
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
    if (code == MOUSE_LEFT_DOUBLE_CLICK)
        return g_windows_input.double_click;

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
        // Clear all input when unfocused
        for (int i = 0; i < INPUT_CODE_COUNT; i++) {
            g_windows_input.key_states[i] = false;
        }
        for (int i = 0; i < XUSER_MAX_COUNT; i++) {
            ZeroMemory(&g_windows_input.gamepad_states[i], sizeof(XINPUT_STATE));
            g_windows_input.gamepad_left_stick[i] = {};
        }
        return;
    }

    bool mouse_over_window = PlatformIsMouseOverWindow();
    bool key_state_changed = false;
    for (int i = 0; i < INPUT_CODE_COUNT; i++) {
        bool old_state = g_windows_input.key_states[i];
        bool new_state = GetAsyncKeyState(g_windows_input.virtual_keys[i]) < 0;
        // Ignore mouse button presses when mouse is not over client area (e.g. window resize)
        if (IsMouse(static_cast<InputCode>(i)) && !mouse_over_window) {
            new_state = false;
        }
        g_windows_input.key_states[i] = new_state;
        key_state_changed |= (old_state != new_state);
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

bool PlatformIsGamepadActive() {
    return g_windows_input.active_controller != -1;
}

void PlatformSetDoubleClick() {
    g_windows_input.double_click = true;
}

void PlatformClearDoubleClick() {
    g_windows_input.double_click = false;
}

bool PlatformWasDoubleClick() {
    return g_windows_input.double_click;
}


HBRUSH GetNativeEditBrush() {
    return g_windows_input.edit_bg_brush;
}

COLORREF GetNativeEditTextColor() {
    return g_windows_input.edit_text_color;
}

COLORREF GetNativeEditBgColor() {
    return g_windows_input.edit_bg_color;
}

void CommitNativeEdit(HWND hwnd) {
    ShowWindow(hwnd, SW_HIDE);
    g_windows_input.edit_visible = false;
}

static COLORREF ColorToColorRef(const Color& c) {
    return RGB(
        static_cast<BYTE>(c.r * 255.0f),
        static_cast<BYTE>(c.g * 255.0f),
        static_cast<BYTE>(c.b * 255.0f)
    );
}

static void UpdateEditFont(int font_size) {
    if (g_windows_input.edit_font_size == font_size)
        return;

    if (g_windows_input.edit_font)
        DeleteObject(g_windows_input.edit_font);

    g_windows_input.edit_font_size = font_size;
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

    SendMessage(g_windows_input.edit_hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(g_windows_input.edit_font), TRUE);
}

void CenterTextbox(int x, int y, int width, int height, UINT flags) {
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

void MarkTextboxChanged() {
    g_windows_input.edit_dirty = true;
}

static void PlatformUpdateTextboxRectInternal(const noz::RectInt& rect, int font_size) {
    UpdateEditFont(font_size);

    if (g_windows_input.edit_rect != rect) {
        g_windows_input.edit_rect = rect;
        SetWindowPos(g_windows_input.edit_hwnd, HWND_TOP, rect.x, rect.y, rect.w, rect.h, SWP_SHOWWINDOW);
        CenterTextbox(rect.x, rect.y, rect.w, rect.h, SWP_SHOWWINDOW);
        InvalidateRect(g_windows_input.edit_hwnd, nullptr, TRUE);
        UpdateWindow(g_windows_input.edit_hwnd);
    }
}

void PlatformUpdateTextboxRect(const noz::Rect& rect, int font_size) {
    int x = static_cast<int>(rect.x);
    int y = static_cast<int>(rect.y);
    int width = static_cast<int>(rect.width);
    int height = static_cast<int>(rect.height);
    PlatformUpdateTextboxRectInternal({x, y, width, height}, font_size);
}

void PlatformShowTextboxInternal(const noz::RectInt& rect, const Text& initial_value, const NativeTextboxStyle& style) {
    // colors
    COLORREF background_color = ColorToColorRef(style.background_color);
    g_windows_input.edit_text_color = ColorToColorRef(style.text_color);
    if (g_windows_input.edit_bg_color != background_color) {
        g_windows_input.edit_bg_color = ColorToColorRef(style.background_color);
        if (!g_windows_input.edit_bg_brush)
            DeleteObject(g_windows_input.edit_bg_brush);
        g_windows_input.edit_bg_brush = CreateSolidBrush(g_windows_input.edit_bg_color);
    }

    // password
    LONG_PTR current_style = GetWindowLongPtr(g_windows_input.edit_hwnd, GWL_STYLE);
    if (style.password) {
        SetWindowLongPtr(g_windows_input.edit_hwnd, GWL_STYLE, current_style | ES_PASSWORD);
        SendMessage(g_windows_input.edit_hwnd, EM_SETPASSWORDCHAR, '*', 0);
    } else {
        SetWindowLongPtr(g_windows_input.edit_hwnd, GWL_STYLE, current_style & ~ES_PASSWORD);
        SendMessage(g_windows_input.edit_hwnd, EM_SETPASSWORDCHAR, 0, 0);
    }

    // Set font and rect BEFORE setting text to ensure correct text layout
    PlatformUpdateTextboxRectInternal(rect, style.font_size);

    // value - set after font so text layout is calculated correctly
    if (initial_value.length > 0) {
        SetWindowTextA(g_windows_input.edit_hwnd, initial_value.value);
        // Reset scroll position to start, then select all
        SendMessage(g_windows_input.edit_hwnd, EM_SETSEL, 0, 0);
        SendMessage(g_windows_input.edit_hwnd, EM_SCROLLCARET, 0, 0);
        SendMessage(g_windows_input.edit_hwnd, EM_SETSEL, 0, -1);
    } else {
        SetWindowTextA(g_windows_input.edit_hwnd, "");
        SendMessage(g_windows_input.edit_hwnd, EM_SETSEL, 0, 0);
        SendMessage(g_windows_input.edit_hwnd, EM_SCROLLCARET, 0, 0);
    }

    SetFocus(g_windows_input.edit_hwnd);

    g_windows_input.edit_visible = true;
}

void PlatformShowTextbox(const noz::Rect& rect, const Text& text, const NativeTextboxStyle& style) {
    int x = static_cast<int>(rect.x);
    int y = static_cast<int>(rect.y);
    int width = static_cast<int>(rect.width);
    int height = static_cast<int>(rect.height);
    PlatformShowTextboxInternal({x, y, width, height}, text, style);
}

void PlatformHideTextbox() {
    ShowWindow(g_windows_input.edit_hwnd, SW_HIDE);
    InvalidateRect(g_windows_input.edit_hwnd, nullptr, TRUE);
    UpdateWindow(g_windows_input.edit_hwnd);
    g_windows_input.edit_visible = false;
    g_windows_input.edit_rect = {0,0,0,0};
}

bool PlatformUpdateTextboxText(Text& text) {
    if (!g_windows_input.edit_dirty)
        return false;

    text.length = GetWindowTextA(g_windows_input.edit_hwnd, text.value, TEXT_MAX_LENGTH);
    g_windows_input.edit_dirty = false;
    return true;
}

bool PlatformIsTextboxVisible() {
    return g_windows_input.edit_visible;
}

static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE || wParam == VK_TAB)) {
        return 0;
    }
    return CallWindowProc(g_windows_input.edit_proc, hwnd, uMsg, wParam, lParam);
}

void PlatformInitInput() {
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

    g_windows_input.edit_hwnd = CreateWindowExA(
        0,
        "EDIT",
        "",
        WS_CHILD | ES_AUTOHSCROLL | ES_LEFT,
        0, 0, 100, 20,
        GetWindowHandle(),
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    // Set margins to 0 immediately to prevent default margins from causing scroll offset
    SendMessage(g_windows_input.edit_hwnd, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));

    g_windows_input.edit_proc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(g_windows_input.edit_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditSubclassProc))
    );
}

void PlatformShutdownInput() {
    DestroyWindow(g_windows_input.edit_hwnd);
    g_windows_input.edit_hwnd = nullptr;
}

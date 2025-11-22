//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../../platform.h"
#include <noz/input_code.h>
#include <Cocoa/Cocoa.h>
#include <GameController/GameController.h>
#include <Carbon/Carbon.h> // For key codes

extern bool GetWindowFocus();

struct MacOSInput {
    Vec2 mouse_scroll;
    bool key_states[INPUT_CODE_COUNT];
    unsigned short key_codes[INPUT_CODE_COUNT];
    GCController* gamepads[4];
    bool gamepad_connected[4];
    Vec2 gamepad_left_stick[4];
    TextInput text_input;
    int active_controller;
};

static MacOSInput g_input = {};

static InputCode KeyCodeToInputCode(unsigned short keyCode) {
    switch (keyCode) {
        case kVK_ANSI_A: return KEY_A;
        case kVK_ANSI_B: return KEY_B;
        case kVK_ANSI_C: return KEY_C;
        case kVK_ANSI_D: return KEY_D;
        case kVK_ANSI_E: return KEY_E;
        case kVK_ANSI_F: return KEY_F;
        case kVK_ANSI_G: return KEY_G;
        case kVK_ANSI_H: return KEY_H;
        case kVK_ANSI_I: return KEY_I;
        case kVK_ANSI_J: return KEY_J;
        case kVK_ANSI_K: return KEY_K;
        case kVK_ANSI_L: return KEY_L;
        case kVK_ANSI_M: return KEY_M;
        case kVK_ANSI_N: return KEY_N;
        case kVK_ANSI_O: return KEY_O;
        case kVK_ANSI_P: return KEY_P;
        case kVK_ANSI_Q: return KEY_Q;
        case kVK_ANSI_R: return KEY_R;
        case kVK_ANSI_S: return KEY_S;
        case kVK_ANSI_T: return KEY_T;
        case kVK_ANSI_U: return KEY_U;
        case kVK_ANSI_V: return KEY_V;
        case kVK_ANSI_W: return KEY_W;
        case kVK_ANSI_X: return KEY_X;
        case kVK_ANSI_Y: return KEY_Y;
        case kVK_ANSI_Z: return KEY_Z;
        case kVK_ANSI_0: return KEY_0;
        case kVK_ANSI_1: return KEY_1;
        case kVK_ANSI_2: return KEY_2;
        case kVK_ANSI_3: return KEY_3;
        case kVK_ANSI_4: return KEY_4;
        case kVK_ANSI_5: return KEY_5;
        case kVK_ANSI_6: return KEY_6;
        case kVK_ANSI_7: return KEY_7;
        case kVK_ANSI_8: return KEY_8;
        case kVK_ANSI_9: return KEY_9;
        case kVK_ANSI_Equal: return KEY_EQUALS;
        case kVK_ANSI_Minus: return KEY_MINUS;
        case kVK_ANSI_Semicolon: return KEY_SEMICOLON;
        case kVK_Space: return KEY_SPACE;
        case kVK_Return: return KEY_ENTER;
        case kVK_Tab: return KEY_TAB;
        case kVK_ANSI_Comma: return KEY_COMMA;
        case kVK_ANSI_Period: return KEY_PERIOD;
        case kVK_Delete: return KEY_BACKSPACE;
        case kVK_Escape: return KEY_ESCAPE;
        case kVK_Shift: return KEY_LEFT_SHIFT;
        case kVK_RightShift: return KEY_RIGHT_SHIFT;
        case kVK_Control: return KEY_LEFT_CTRL;
        case kVK_RightControl: return KEY_RIGHT_CTRL;
        case kVK_Option: return KEY_LEFT_ALT;
        case kVK_RightOption: return KEY_RIGHT_ALT;
        case kVK_UpArrow: return KEY_UP;
        case kVK_DownArrow: return KEY_DOWN;
        case kVK_LeftArrow: return KEY_LEFT;
        case kVK_RightArrow: return KEY_RIGHT;
        case kVK_F1: return KEY_F1;
        case kVK_F2: return KEY_F2;
        case kVK_F3: return KEY_F3;
        case kVK_F4: return KEY_F4;
        case kVK_F5: return KEY_F5;
        case kVK_F6: return KEY_F6;
        case kVK_F7: return KEY_F7;
        case kVK_F8: return KEY_F8;
        case kVK_F9: return KEY_F9;
        case kVK_F10: return KEY_F10;
        case kVK_F11: return KEY_F11;
        case kVK_F12: return KEY_F12;
        case kVK_ANSI_LeftBracket: return KEY_LEFT_BRACKET;
        case kVK_ANSI_RightBracket: return KEY_RIGHT_BRACKET;
        default: return INPUT_CODE_NONE;
    }
}

static float NormalizeStick(float value, float deadzone)
{
    if (value < -deadzone)
        return (value + deadzone) / (1.0f - deadzone);
    if (value > deadzone)
        return (value - deadzone) / (1.0f - deadzone);
    return 0.0f;
}

static InputCode GetLeftStickButton(int gamepad_index) {
    const Vec2& stick = g_input.gamepad_left_stick[gamepad_index];
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

bool platform::IsInputButtonDown(InputCode code) {
    if (IsMouse(code) || IsKeyboard(code))
        return g_input.key_states[code];

    // Handle gamepad buttons
    if (IsGamepad(code) && IsButton(code)) {
        int gamepad_index = 0;

        // Determine gamepad index
        if (code >= GAMEPAD_A && code <= GAMEPAD_RIGHT_TRIGGER) {
            for (int i = 0; i < 4; i++) {
                if (g_input.gamepad_connected[i]) {
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

        if (!g_input.gamepad_connected[gamepad_index])
            return false;

        GCController* controller = g_input.gamepads[gamepad_index];
        if (!controller || !controller.extendedGamepad)
            return false;

        GCExtendedGamepad* gamepad = controller.extendedGamepad;

        switch (code) {
            case GAMEPAD_A: return gamepad.buttonA.pressed;
            case GAMEPAD_B: return gamepad.buttonB.pressed;
            case GAMEPAD_X: return gamepad.buttonX.pressed;
            case GAMEPAD_Y: return gamepad.buttonY.pressed;
            case GAMEPAD_LEFT_SHOULDER: return gamepad.leftShoulder.pressed;
            case GAMEPAD_RIGHT_SHOULDER: return gamepad.rightShoulder.pressed;
            case GAMEPAD_START: return gamepad.buttonMenu.pressed;
            case GAMEPAD_BACK: return gamepad.buttonOptions ? gamepad.buttonOptions.pressed : false;
            case GAMEPAD_LEFT_STICK_BUTTON: return gamepad.leftThumbstickButton ? gamepad.leftThumbstickButton.pressed : false;
            case GAMEPAD_RIGHT_STICK_BUTTON: return gamepad.rightThumbstickButton ? gamepad.rightThumbstickButton.pressed : false;
            case GAMEPAD_DPAD_UP: return gamepad.dpad.up.pressed;
            case GAMEPAD_DPAD_DOWN: return gamepad.dpad.down.pressed;
            case GAMEPAD_DPAD_LEFT: return gamepad.dpad.left.pressed;
            case GAMEPAD_DPAD_RIGHT: return gamepad.dpad.right.pressed;
            case GAMEPAD_LEFT_TRIGGER_BUTTON: return gamepad.leftTrigger.value > 0.5f;
            case GAMEPAD_RIGHT_TRIGGER_BUTTON: return gamepad.rightTrigger.value > 0.5f;
            case GAMEPAD_LEFT_STICK_LEFT:
            case GAMEPAD_LEFT_STICK_RIGHT:
            case GAMEPAD_LEFT_STICK_UP:
            case GAMEPAD_LEFT_STICK_DOWN:
                return GetLeftStickButton(gamepad_index) == code;
            default: return false;
        }
    }

    return false;
}

static bool WasGamepadUsed(int gamepad_index) {
    if (!g_input.gamepad_connected[gamepad_index])
        return false;

    GCController* controller = g_input.gamepads[gamepad_index];
    if (!controller || !controller.extendedGamepad)
        return false;

    GCExtendedGamepad* gamepad = controller.extendedGamepad;

    if (fabsf(gamepad.leftThumbstick.xAxis.value) > 0.1f)
        return true;
    if (fabsf(gamepad.leftThumbstick.yAxis.value) > 0.1f)
        return true;
    if (fabsf(gamepad.rightThumbstick.xAxis.value) > 0.1f)
        return true;
    if (fabsf(gamepad.rightThumbstick.yAxis.value) > 0.1f)
        return true;
    if (gamepad.leftTrigger.value > 0.1f)
        return true;
    if (gamepad.rightTrigger.value > 0.1f)
        return true;

    return false;
}

float platform::GetInputAxisValue(InputCode code) {
    // Handle mouse axes
    if (IsMouse(code) && IsAxis(code)) {
        switch (code) {
            case MOUSE_X: return GetMousePosition().x;
            case MOUSE_Y: return GetMousePosition().y;
            case MOUSE_SCROLL_X: return GetMouseScroll().x;
            case MOUSE_SCROLL_Y: return GetMouseScroll().y;
            default: return 0.0f;
        }
    }

    // Handle gamepad axes
    if (IsGamepad(code) && IsAxis(code)) {
        int gamepad_index = 0;

        // Determine gamepad index
        if (code >= GAMEPAD_LEFT_STICK_X && code <= GAMEPAD_RIGHT_TRIGGER) {
            for (int i = 0; i < 4; i++) {
                if (g_input.gamepad_connected[i]) {
                    gamepad_index = i;
                    break;
                }
            }
        } else if (code >= GAMEPAD_1_LEFT_STICK_X && code <= GAMEPAD_1_RIGHT_TRIGGER) {
            gamepad_index = 0;
            code = static_cast<InputCode>(code - GAMEPAD_1_LEFT_STICK_X + GAMEPAD_LEFT_STICK_X);
        } else if (code >= GAMEPAD_2_LEFT_STICK_X && code <= GAMEPAD_2_RIGHT_TRIGGER) {
            gamepad_index = 1;
            code = static_cast<InputCode>(code - GAMEPAD_2_LEFT_STICK_X + GAMEPAD_LEFT_STICK_X);
        } else if (code >= GAMEPAD_3_LEFT_STICK_X && code <= GAMEPAD_3_RIGHT_TRIGGER) {
            gamepad_index = 2;
            code = static_cast<InputCode>(code - GAMEPAD_3_LEFT_STICK_X + GAMEPAD_LEFT_STICK_X);
        } else if (code >= GAMEPAD_4_LEFT_STICK_X && code <= GAMEPAD_4_RIGHT_TRIGGER) {
            gamepad_index = 3;
            code = static_cast<InputCode>(code - GAMEPAD_4_LEFT_STICK_X + GAMEPAD_LEFT_STICK_X);
        }

        if (!g_input.gamepad_connected[gamepad_index])
            return 0.0f;

        GCController* controller = g_input.gamepads[gamepad_index];
        if (!controller || !controller.extendedGamepad)
            return 0.0f;

        GCExtendedGamepad* gamepad = controller.extendedGamepad;

        switch (code) {
            case GAMEPAD_LEFT_STICK_X:
                return NormalizeStick(gamepad.leftThumbstick.xAxis.value, 0.1f);
            case GAMEPAD_LEFT_STICK_Y:
                return NormalizeStick(gamepad.leftThumbstick.yAxis.value, 0.1f);
            case GAMEPAD_RIGHT_STICK_X:
                return NormalizeStick(gamepad.rightThumbstick.xAxis.value, 0.1f);
            case GAMEPAD_RIGHT_STICK_Y:
                return NormalizeStick(gamepad.rightThumbstick.yAxis.value, 0.1f);
            case GAMEPAD_LEFT_TRIGGER:
                return gamepad.leftTrigger.value;
            case GAMEPAD_RIGHT_TRIGGER:
                return gamepad.rightTrigger.value;
            default:
                return 0.0f;
        }
    }

    return 0.0f;
}

void platform::UpdateInputState() {
    if (!HasFocus()) {
        for (int i = 0; i < INPUT_CODE_COUNT; i++)
            g_input.key_states[i] = false;
        g_input.mouse_scroll = {0, 0};
        return;
    }

    // Update keyboard and mouse state
    bool key_state_changed = false;
    for (int i = 0; i < INPUT_CODE_COUNT; i++) {
        if (g_input.key_codes[i] != 0) {
            bool old_state = g_input.key_states[i];
            // For keyboard keys
            if (i >= KEY_A && i <= KEY_RIGHT_BRACKET) {
                g_input.key_states[i] = CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, g_input.key_codes[i]);
            }
            // For mouse buttons
            else if (i == MOUSE_LEFT) {
                g_input.key_states[i] = CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft);
            }
            else if (i == MOUSE_RIGHT) {
                g_input.key_states[i] = CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonRight);
            }
            else if (i == MOUSE_MIDDLE) {
                g_input.key_states[i] = CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonCenter);
            }
            key_state_changed |= (old_state != g_input.key_states[i]);
        }
    }

    // Update gamepad state
    for (int i = 0; i < 4; i++) {
        if (g_input.gamepad_connected[i]) {
            GCController* controller = g_input.gamepads[i];
            if (controller && controller.extendedGamepad) {
                GCExtendedGamepad* gamepad = controller.extendedGamepad;
                g_input.gamepad_left_stick[i] = Vec2{
                    NormalizeStick(gamepad.leftThumbstick.xAxis.value, 0.1f),
                    NormalizeStick(gamepad.leftThumbstick.yAxis.value, 0.1f)
                };

                if (g_input.active_controller != i && WasGamepadUsed(i)) {
                    g_input.active_controller = i;
                }
            }
        }
    }

    if (key_state_changed) {
        g_input.active_controller = -1;
    }
}

void HandleInputCharacter(unichar c) {
    if (!IsTextInputEnabled())
        return;

    if (g_input.text_input.length >= TEXT_INPUT_MAX_LENGTH - 1)
        return;

    if (c == 27) { // Escape
        Send(EVENT_TEXTINPUT_CANCEL, &g_input.text_input);
        return;
    }

    if (c == 13) { // Enter
        Send(EVENT_TEXTINPUT_COMMIT, &g_input.text_input);
        return;
    }

    if (c < 32)
        return;

    g_input.text_input.value[g_input.text_input.length++] = (char)c;
    g_input.text_input.value[g_input.text_input.length] = 0;
    g_input.text_input.cursor++;

    Send(EVENT_TEXTINPUT_CHANGE, &g_input.text_input);
}

void HandleInputKeyDown(unsigned short keyCode) {
    if (!IsTextInputEnabled())
        return;

    if (keyCode == kVK_Delete) { // Backspace
        if (g_input.text_input.length == 0 || g_input.text_input.cursor == 0)
            return;

        // Move characters back one position from cursor
        for (int i = g_input.text_input.cursor - 1; i < g_input.text_input.length - 1; i++)
            g_input.text_input.value[i] = g_input.text_input.value[i + 1];
        g_input.text_input.length--;
        g_input.text_input.value[g_input.text_input.length] = 0;
        g_input.text_input.cursor--;
        Send(EVENT_TEXTINPUT_CHANGE, &g_input.text_input);
        return;
    }
}

void platform::ClearTextInput() {
    if (g_input.text_input.length == 0)
        return;

    g_input.text_input.value[0] = 0;
    g_input.text_input.length = 0;
    g_input.text_input.cursor = 0;
    Send(EVENT_TEXTINPUT_CHANGE, &g_input.text_input);
}

const TextInput& platform::GetTextInput() {
    return g_input.text_input;
}

void platform::SetTextInput(const TextInput& text_input) {
    g_input.text_input = text_input;
}

bool platform::IsGamepadActive() {
    return g_input.active_controller != -1;
}

void platform::InitializeInput() {
    ClearTextInput();

    // Initialize gamepad controllers
    @autoreleasepool {
        NSArray<GCController*>* controllers = [GCController controllers];
        for (int i = 0; i < (int)[controllers count] && i < 4; i++) {
            g_input.gamepads[i] = controllers[i];
            g_input.gamepad_connected[i] = true;
        }

        // Set up controller connection/disconnection notifications
        [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification
                                                           object:nil
                                                            queue:nil
                                                       usingBlock:^(NSNotification* note) {
            GCController* controller = note.object;
            for (int i = 0; i < 4; i++) {
                if (!g_input.gamepad_connected[i]) {
                    g_input.gamepads[i] = controller;
                    g_input.gamepad_connected[i] = true;
                    break;
                }
            }
        }];

        [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification
                                                           object:nil
                                                            queue:nil
                                                       usingBlock:^(NSNotification* note) {
            GCController* controller = note.object;
            for (int i = 0; i < 4; i++) {
                if (g_input.gamepads[i] == controller) {
                    g_input.gamepads[i] = nil;
                    g_input.gamepad_connected[i] = false;
                    if (g_input.active_controller == i) {
                        g_input.active_controller = -1;
                    }
                    break;
                }
            }
        }];
    }

    // Build key code mapping
    for (unsigned short keyCode = 0; keyCode < 256; keyCode++) {
        InputCode code = KeyCodeToInputCode(keyCode);
        if (code == INPUT_CODE_NONE)
            continue;
        g_input.key_codes[code] = keyCode;
    }

    // Set up mouse button codes (using special markers)
    g_input.key_codes[MOUSE_LEFT] = 1;
    g_input.key_codes[MOUSE_RIGHT] = 1;
    g_input.key_codes[MOUSE_MIDDLE] = 1;
}

void platform::ShutdownInput() {
    @autoreleasepool {
        [[NSNotificationCenter defaultCenter] removeObserver:nil
                                                         name:GCControllerDidConnectNotification
                                                       object:nil];
        [[NSNotificationCenter defaultCenter] removeObserver:nil
                                                         name:GCControllerDidDisconnectNotification
                                                       object:nil];
    }

    for (int i = 0; i < 4; i++) {
        g_input.gamepads[i] = nil;
        g_input.gamepad_connected[i] = false;
    }
}

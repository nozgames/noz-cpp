//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//
//  Web/Emscripten input handling
//

#include "../../platform.h"
#include "../../internal.h"
#include <noz/input_code.h>

#include <emscripten.h>
#include <emscripten/html5.h>

struct WebInput {
    bool key_states[INPUT_CODE_COUNT];
    bool gamepad_connected[4];
    double gamepad_axes[4][6];  // 4 gamepads, 6 axes each
    bool gamepad_buttons[4][20]; // 4 gamepads, 20 buttons each
    int active_controller;
};

static WebInput g_web_input = {};

// Functions needed by web_main.cpp for touch input simulation
bool* GetKeyStates() {
    return g_web_input.key_states;
}

void HandleInputKeyDown(char c) {
    // Handle special keys for text input (unused with native textbox)
    (void)c;
}

void HandleInputCharacter(char c) {
    // Handle character input (unused with native textbox)
    (void)c;
}

// Map browser key codes to InputCode
static InputCode KeyCodeToInputCode(const char* code) {
    // Letters
    if (code[0] == 'K' && code[1] == 'e' && code[2] == 'y') {
        char letter = code[3];
        if (letter >= 'A' && letter <= 'Z') {
            return static_cast<InputCode>(KEY_A + (letter - 'A'));
        }
    }

    // Digits
    if (code[0] == 'D' && code[1] == 'i' && code[2] == 'g' && code[3] == 'i' && code[4] == 't') {
        char digit = code[5];
        if (digit >= '0' && digit <= '9') {
            return static_cast<InputCode>(KEY_0 + (digit - '0'));
        }
    }

    // Special keys
    if (strcmp(code, "Space") == 0) return KEY_SPACE;
    if (strcmp(code, "Enter") == 0) return KEY_ENTER;
    if (strcmp(code, "Tab") == 0) return KEY_TAB;
    if (strcmp(code, "Backspace") == 0) return KEY_BACKSPACE;
    if (strcmp(code, "Escape") == 0) return KEY_ESCAPE;
    if (strcmp(code, "ShiftLeft") == 0) return KEY_LEFT_SHIFT;
    if (strcmp(code, "ShiftRight") == 0) return KEY_RIGHT_SHIFT;
    if (strcmp(code, "ControlLeft") == 0) return KEY_LEFT_CTRL;
    if (strcmp(code, "ControlRight") == 0) return KEY_RIGHT_CTRL;
    if (strcmp(code, "AltLeft") == 0) return KEY_LEFT_ALT;
    if (strcmp(code, "AltRight") == 0) return KEY_RIGHT_ALT;
    if (strcmp(code, "ArrowUp") == 0) return KEY_UP;
    if (strcmp(code, "ArrowDown") == 0) return KEY_DOWN;
    if (strcmp(code, "ArrowLeft") == 0) return KEY_LEFT;
    if (strcmp(code, "ArrowRight") == 0) return KEY_RIGHT;
    if (strcmp(code, "Comma") == 0) return KEY_COMMA;
    if (strcmp(code, "Period") == 0) return KEY_PERIOD;
    if (strcmp(code, "Semicolon") == 0) return KEY_SEMICOLON;
    if (strcmp(code, "Quote") == 0) return KEY_QUOTE;
    if (strcmp(code, "Minus") == 0) return KEY_MINUS;
    if (strcmp(code, "Equal") == 0) return KEY_EQUALS;
    if (strcmp(code, "Backquote") == 0) return KEY_TILDE;
    if (strcmp(code, "BracketLeft") == 0) return KEY_LEFT_BRACKET;
    if (strcmp(code, "BracketRight") == 0) return KEY_RIGHT_BRACKET;

    // Function keys
    if (strcmp(code, "F1") == 0) return KEY_F1;
    if (strcmp(code, "F2") == 0) return KEY_F2;
    if (strcmp(code, "F3") == 0) return KEY_F3;
    if (strcmp(code, "F4") == 0) return KEY_F4;
    if (strcmp(code, "F5") == 0) return KEY_F5;
    if (strcmp(code, "F6") == 0) return KEY_F6;
    if (strcmp(code, "F7") == 0) return KEY_F7;
    if (strcmp(code, "F8") == 0) return KEY_F8;
    if (strcmp(code, "F9") == 0) return KEY_F9;
    if (strcmp(code, "F10") == 0) return KEY_F10;
    if (strcmp(code, "F11") == 0) return KEY_F11;
    if (strcmp(code, "F12") == 0) return KEY_F12;

    return INPUT_CODE_NONE;
}

// Keyboard event handlers
static EM_BOOL OnKeyDownInput(int event_type, const EmscriptenKeyboardEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    InputCode code = KeyCodeToInputCode(event->code);
    if (code != INPUT_CODE_NONE) {
        g_web_input.key_states[code] = true;
        g_web_input.active_controller = -1; // Keyboard used, disable gamepad mode
    }

    return EM_FALSE;
}

static EM_BOOL OnKeyUpInput(int event_type, const EmscriptenKeyboardEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    InputCode code = KeyCodeToInputCode(event->code);
    if (code != INPUT_CODE_NONE) {
        g_web_input.key_states[code] = false;
    }

    return EM_FALSE;
}

// Mouse event handlers
static EM_BOOL OnMouseDown(int event_type, const EmscriptenMouseEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    switch (event->button) {
        case 0: g_web_input.key_states[MOUSE_LEFT] = true; break;
        case 1: g_web_input.key_states[MOUSE_MIDDLE] = true; break;
        case 2: g_web_input.key_states[MOUSE_RIGHT] = true; break;
        case 3: g_web_input.key_states[MOUSE_BUTTON_4] = true; break;
        case 4: g_web_input.key_states[MOUSE_BUTTON_5] = true; break;
    }

    return EM_FALSE;
}

static EM_BOOL OnMouseUp(int event_type, const EmscriptenMouseEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    switch (event->button) {
        case 0: g_web_input.key_states[MOUSE_LEFT] = false; break;
        case 1: g_web_input.key_states[MOUSE_MIDDLE] = false; break;
        case 2: g_web_input.key_states[MOUSE_RIGHT] = false; break;
        case 3: g_web_input.key_states[MOUSE_BUTTON_4] = false; break;
        case 4: g_web_input.key_states[MOUSE_BUTTON_5] = false; break;
    }

    return EM_FALSE;
}

// Gamepad event handlers
static EM_BOOL OnGamepadConnected(int event_type, const EmscriptenGamepadEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    if (event->index >= 0 && event->index < 4) {
        g_web_input.gamepad_connected[event->index] = true;
        LogInfo("Gamepad connected: %s (index %d)", event->id, event->index);
    }

    return EM_TRUE;
}

static EM_BOOL OnGamepadDisconnected(int event_type, const EmscriptenGamepadEvent* event, void* user_data) {
    (void)event_type;
    (void)user_data;

    if (event->index >= 0 && event->index < 4) {
        g_web_input.gamepad_connected[event->index] = false;
        if (g_web_input.active_controller == event->index) {
            g_web_input.active_controller = -1;
        }
        LogInfo("Gamepad disconnected: %s (index %d)", event->id, event->index);
    }

    return EM_TRUE;
}

static void UpdateGamepads() {
    // Sample gamepad state
    if (emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS) {
        return;
    }

    int num_gamepads = emscripten_get_num_gamepads();
    for (int i = 0; i < 4 && i < num_gamepads; i++) {
        EmscriptenGamepadEvent state;
        if (emscripten_get_gamepad_status(i, &state) == EMSCRIPTEN_RESULT_SUCCESS) {
            g_web_input.gamepad_connected[i] = state.connected;

            if (state.connected) {
                // Update axes
                for (int j = 0; j < 6 && j < state.numAxes; j++) {
                    double old_value = g_web_input.gamepad_axes[i][j];
                    g_web_input.gamepad_axes[i][j] = state.axis[j];

                    // Check if gamepad was used
                    if (fabs(state.axis[j]) > 0.2 && fabs(old_value) <= 0.2) {
                        g_web_input.active_controller = i;
                    }
                }

                // Update buttons
                for (int j = 0; j < 20 && j < state.numButtons; j++) {
                    bool old_state = g_web_input.gamepad_buttons[i][j];
                    g_web_input.gamepad_buttons[i][j] = state.digitalButton[j];

                    // Check if gamepad was used
                    if (state.digitalButton[j] && !old_state) {
                        g_web_input.active_controller = i;
                    }
                }
            }
        }
    }
}

// Map standard gamepad button index to InputCode
static bool GetGamepadButtonState(int gamepad_index, InputCode code) {
    if (!g_web_input.gamepad_connected[gamepad_index]) {
        return false;
    }

    // Standard gamepad mapping (W3C Standard Gamepad)
    // https://w3c.github.io/gamepad/#remapping
    int button_index = -1;
    switch (code) {
        case GAMEPAD_A: button_index = 0; break;  // Bottom face button
        case GAMEPAD_B: button_index = 1; break;  // Right face button
        case GAMEPAD_X: button_index = 2; break;  // Left face button
        case GAMEPAD_Y: button_index = 3; break;  // Top face button
        case GAMEPAD_LEFT_SHOULDER: button_index = 4; break;
        case GAMEPAD_RIGHT_SHOULDER: button_index = 5; break;
        case GAMEPAD_LEFT_TRIGGER_BUTTON: button_index = 6; break;
        case GAMEPAD_RIGHT_TRIGGER_BUTTON: button_index = 7; break;
        case GAMEPAD_BACK: button_index = 8; break;
        case GAMEPAD_START: button_index = 9; break;
        case GAMEPAD_LEFT_STICK_BUTTON: button_index = 10; break;
        case GAMEPAD_RIGHT_STICK_BUTTON: button_index = 11; break;
        case GAMEPAD_DPAD_UP: button_index = 12; break;
        case GAMEPAD_DPAD_DOWN: button_index = 13; break;
        case GAMEPAD_DPAD_LEFT: button_index = 14; break;
        case GAMEPAD_DPAD_RIGHT: button_index = 15; break;
        case GAMEPAD_GUIDE: button_index = 16; break;
        default: return false;
    }

    if (button_index >= 0 && button_index < 20) {
        return g_web_input.gamepad_buttons[gamepad_index][button_index];
    }

    return false;
}

static float GetGamepadAxisValue(int gamepad_index, InputCode code) {
    if (!g_web_input.gamepad_connected[gamepad_index]) {
        return 0.0f;
    }

    // Standard gamepad mapping
    // Axis 0: Left stick X
    // Axis 1: Left stick Y
    // Axis 2: Right stick X
    // Axis 3: Right stick Y
    // Triggers may be axis 4/5 or buttons 6/7 depending on browser
    int axis_index = -1;
    float sign = 1.0f;

    switch (code) {
        case GAMEPAD_LEFT_STICK_X: axis_index = 0; break;
        case GAMEPAD_LEFT_STICK_Y: axis_index = 1; sign = -1.0f; break;  // Invert Y
        case GAMEPAD_RIGHT_STICK_X: axis_index = 2; break;
        case GAMEPAD_RIGHT_STICK_Y: axis_index = 3; sign = -1.0f; break;  // Invert Y
        case GAMEPAD_LEFT_TRIGGER: axis_index = 4; break;
        case GAMEPAD_RIGHT_TRIGGER: axis_index = 5; break;
        default: return 0.0f;
    }

    if (axis_index >= 0 && axis_index < 6) {
        float value = (float)g_web_input.gamepad_axes[gamepad_index][axis_index] * sign;

        // Apply deadzone
        const float deadzone = 0.15f;
        if (fabs(value) < deadzone) {
            return 0.0f;
        }

        // Normalize after deadzone
        if (value > 0) {
            return (value - deadzone) / (1.0f - deadzone);
        } else {
            return (value + deadzone) / (1.0f - deadzone);
        }
    }

    return 0.0f;
}

// Emulate stick as buttons
static bool GetStickAsButton(int gamepad_index, InputCode code) {
    float threshold = 0.5f;

    switch (code) {
        case GAMEPAD_LEFT_STICK_LEFT:
            return GetGamepadAxisValue(gamepad_index, GAMEPAD_LEFT_STICK_X) < -threshold;
        case GAMEPAD_LEFT_STICK_RIGHT:
            return GetGamepadAxisValue(gamepad_index, GAMEPAD_LEFT_STICK_X) > threshold;
        case GAMEPAD_LEFT_STICK_UP:
            return GetGamepadAxisValue(gamepad_index, GAMEPAD_LEFT_STICK_Y) > threshold;
        case GAMEPAD_LEFT_STICK_DOWN:
            return GetGamepadAxisValue(gamepad_index, GAMEPAD_LEFT_STICK_Y) < -threshold;
        default:
            return false;
    }
}

bool PlatformIsInputButtonDown(InputCode code) {
    if (IsMouse(code) || IsKeyboard(code)) {
        return g_web_input.key_states[code];
    }

    // Handle gamepad buttons
    if (IsGamepad(code) && IsButton(code)) {
        int gamepad_index = 0;

        // Determine gamepad index and normalize code
        if (code >= GAMEPAD_A && code <= GAMEPAD_RIGHT_TRIGGER_BUTTON) {
            // Generic gamepad - use first connected
            for (int i = 0; i < 4; i++) {
                if (g_web_input.gamepad_connected[i]) {
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

        // Handle stick-as-button
        if (code >= GAMEPAD_LEFT_STICK_LEFT && code <= GAMEPAD_LEFT_STICK_DOWN) {
            return GetStickAsButton(gamepad_index, code);
        }

        return GetGamepadButtonState(gamepad_index, code);
    }

    return false;
}

float PlatformGetInputAxisValue(InputCode code) {
    // Handle mouse axes
    if (IsMouse(code) && IsAxis(code)) {
        switch (code) {
            case MOUSE_X: return PlatformGetMousePosition().x;
            case MOUSE_Y: return PlatformGetMousePosition().y;
            case MOUSE_SCROLL_X: return PlatformGetMouseScroll().x;
            case MOUSE_SCROLL_Y: return PlatformGetMouseScroll().y;
            default: return 0.0f;
        }
    }

    // Handle gamepad axes
    if (IsGamepad(code) && IsAxis(code)) {
        int gamepad_index = 0;

        // Determine gamepad index and normalize code
        if (code >= GAMEPAD_LEFT_STICK_X && code <= GAMEPAD_RIGHT_TRIGGER) {
            // Generic gamepad - use first connected
            for (int i = 0; i < 4; i++) {
                if (g_web_input.gamepad_connected[i]) {
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

        return GetGamepadAxisValue(gamepad_index, code);
    }

    return 0.0f;
}

void PlatformUpdateInputState() {
    if (!PlatformIsWindowFocused()) {
        // Clear all input when unfocused
        for (int i = 0; i < INPUT_CODE_COUNT; i++) {
            g_web_input.key_states[i] = false;
        }

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 6; j++) {
                g_web_input.gamepad_axes[i][j] = 0.0;
            }
            for (int j = 0; j < 20; j++) {
                g_web_input.gamepad_buttons[i][j] = false;
            }
        }

        return;
    }

    // Update gamepad state
    UpdateGamepads();
}

bool PlatformIsGamepadActive() {
    return g_web_input.active_controller != -1;
}

void PlatformInitInput() {
    g_web_input = {};
    g_web_input.active_controller = -1;

    // Register keyboard events
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, OnKeyDownInput);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, OnKeyUpInput);

    // Register mouse events
    emscripten_set_mousedown_callback("#canvas", nullptr, EM_TRUE, OnMouseDown);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, nullptr, EM_TRUE, OnMouseUp);

    // Register gamepad events
    emscripten_set_gamepadconnected_callback(nullptr, EM_TRUE, OnGamepadConnected);
    emscripten_set_gamepaddisconnected_callback(nullptr, EM_TRUE, OnGamepadDisconnected);
}

void PlatformShutdownInput() {
    // Nothing to cleanup
}

// Native text input overlay
static bool g_native_text_input_visible = false;
static bool g_native_text_input_dirty = false;
static char g_native_text_input_value[TEXT_MAX_LENGTH + 1] = {};
static noz::Rect g_native_text_input_rect = {};
static int g_native_text_input_font_size = 0;

static void ColorToRgbString(const Color& c, char* out, int max_len) {
    snprintf(out, max_len, "rgb(%d,%d,%d)",
        static_cast<int>(c.r * 255.0f),
        static_cast<int>(c.g * 255.0f),
        static_cast<int>(c.b * 255.0f));
}

void PlatformShowTextbox(const noz::Rect& rect, const Text& text, const NativeTextboxStyle& style) {
    char bg_color[32];
    char text_color[32];
    ColorToRgbString(style.background_color, bg_color, sizeof(bg_color));
    ColorToRgbString(style.text_color, text_color, sizeof(text_color));

    g_native_text_input_visible = true;
    g_native_text_input_dirty = false;
    g_native_text_input_rect = rect;
    g_native_text_input_font_size = style.font_size;

    if (text.length > 0) {
        strncpy(g_native_text_input_value, text.value, TEXT_MAX_LENGTH);
        g_native_text_input_value[TEXT_MAX_LENGTH] = 0;
    } else {
        g_native_text_input_value[0] = 0;
    }

    const char* input_type = style.password ? "password" : "text";

    EM_ASM({
        var input = document.getElementById('native-text-input');
        if (!input) {
            input = document.createElement('input');
            input.id = 'native-text-input';
            input.autocomplete = 'off';
            input.style.position = 'absolute';
            input.style.zIndex = '1000';
            input.style.outline = 'none';
            input.style.border = 'none';
            input.style.fontFamily = 'sans-serif';
            input.style.padding = '0';
            input.style.boxSizing = 'border-box';
            document.body.appendChild(input);

            input.addEventListener('input', function() {
                Module.ccall('NativeTextInputChanged', null, ['string'], [input.value]);
            });

            input.addEventListener('keydown', function(e) {
                if (e.key === 'Enter') {
                    Module.ccall('NativeTextInputCommit', null, [], []);
                    e.preventDefault();
                } else if (e.key === 'Escape') {
                    Module.ccall('NativeTextInputCancel', null, [], []);
                    e.preventDefault();
                } else if (e.key === 'Tab') {
                    e.preventDefault(); // Stop browser focus change, but let game see it
                    return;
                }
                e.stopPropagation();
            });

            input.addEventListener('keyup', function(e) {
                e.stopPropagation();
            });

            input.addEventListener('keypress', function(e) {
                e.stopPropagation();
            });
        }

        // Get canvas position and device pixel ratio to convert canvas coords to page coords
        var canvas = document.getElementById('canvas');
        var canvasLeft = 0;
        var canvasTop = 0;
        if (canvas) {
            var canvasRect = canvas.getBoundingClientRect();
            canvasLeft = canvasRect.left;
            canvasTop = canvasRect.top;
        }
        var dpr = window.devicePixelRatio || 1;

        // Convert from canvas buffer coordinates to CSS pixels, then add canvas offset
        var cssX = ($0 / dpr) + canvasLeft;
        var cssY = ($1 / dpr) + canvasTop;
        var cssWidth = $2 / dpr;
        var cssHeight = $3 / dpr;
        var cssFontSize = $4 / dpr;

        input.type = UTF8ToString($8);
        input.style.left = cssX + 'px';
        input.style.top = cssY + 'px';
        input.style.width = cssWidth + 'px';
        input.style.height = cssHeight + 'px';
        input.style.fontSize = cssFontSize + 'px';
        input.style.backgroundColor = UTF8ToString($5);
        input.style.color = UTF8ToString($6);
        input.value = UTF8ToString($7);
        input.style.display = 'block';
        input.focus();
        input.select();
    }, rect.x, rect.y, rect.width, rect.height, style.font_size, bg_color, text_color, text.value, input_type);
}

void PlatformUpdateTextboxRect(const noz::Rect& rect, int font_size) {
    if (!g_native_text_input_visible)
        return;

    // Only update if rect or font_size changed
    if (g_native_text_input_rect.x == rect.x &&
        g_native_text_input_rect.y == rect.y &&
        g_native_text_input_rect.width == rect.width &&
        g_native_text_input_rect.height == rect.height &&
        g_native_text_input_font_size == font_size) {
        return;
    }

    g_native_text_input_rect = rect;
    g_native_text_input_font_size = font_size;

    EM_ASM({
        var input = document.getElementById('native-text-input');
        if (input) {
            // Get canvas position and device pixel ratio to convert canvas coords to page coords
            var canvas = document.getElementById('canvas');
            var canvasLeft = 0;
            var canvasTop = 0;
            if (canvas) {
                var canvasRect = canvas.getBoundingClientRect();
                canvasLeft = canvasRect.left;
                canvasTop = canvasRect.top;
            }
            var dpr = window.devicePixelRatio || 1;

            var cssX = ($0 / dpr) + canvasLeft;
            var cssY = ($1 / dpr) + canvasTop;
            var cssWidth = $2 / dpr;
            var cssHeight = $3 / dpr;
            var cssFontSize = $4 / dpr;

            input.style.left = cssX + 'px';
            input.style.top = cssY + 'px';
            input.style.width = cssWidth + 'px';
            input.style.height = cssHeight + 'px';
            input.style.fontSize = cssFontSize + 'px';
        }
    }, rect.x, rect.y, rect.width, rect.height, font_size);
}

void PlatformHideTextbox() {
    if (!g_native_text_input_visible)
        return;

    g_native_text_input_visible = false;
    g_native_text_input_rect = {0, 0, 0, 0};

    EM_ASM({
        var input = document.getElementById('native-text-input');
        if (input) {
            input.style.display = 'none';
            input.blur();
        }
        // Return focus to canvas
        var canvas = document.getElementById('canvas');
        if (canvas) canvas.focus();
    });
}

bool PlatformIsTextboxVisible() {
    return g_native_text_input_visible;
}

bool PlatformUpdateTextboxText(Text& text) {
    if (!g_native_text_input_dirty)
        return false;

    size_t len = strlen(g_native_text_input_value);
    if (len > TEXT_MAX_LENGTH)
        len = TEXT_MAX_LENGTH;

    memcpy(text.value, g_native_text_input_value, len);
    text.value[len] = 0;
    text.length = static_cast<int>(len);

    g_native_text_input_dirty = false;
    return true;
}

// C callbacks for JavaScript
extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void NativeTextInputChanged(const char* value) {
        if (value) {
            strncpy(g_native_text_input_value, value, TEXT_MAX_LENGTH);
            g_native_text_input_value[TEXT_MAX_LENGTH] = 0;
        }
        g_native_text_input_dirty = true;
    }

    EMSCRIPTEN_KEEPALIVE
    void NativeTextInputCommit() {
        PlatformHideTextbox();
    }

    EMSCRIPTEN_KEEPALIVE
    void NativeTextInputCancel() {
        PlatformHideTextbox();
    }
}

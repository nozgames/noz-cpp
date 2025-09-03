//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if 0
static InputCode g_scancode_to_input_code[SDL_SCANCODE_COUNT];
static SDL_Scancode g_input_code_to_scancode[INPUT_CODE_COUNT];

static void InitScanCodeTable(InputCode* table)
{
    for (int i = 0; i < SDL_SCANCODE_COUNT; ++i)
        table[i] = INPUT_CODE_NONE;

    table[SDL_SCANCODE_A]           = KEY_A;
    table[SDL_SCANCODE_B]           = KEY_B;
    table[SDL_SCANCODE_C]           = KEY_C;
    table[SDL_SCANCODE_D]           = KEY_D;
    table[SDL_SCANCODE_E]           = KEY_E;
    table[SDL_SCANCODE_F]           = KEY_F;
    table[SDL_SCANCODE_G]           = KEY_G;
    table[SDL_SCANCODE_H]           = KEY_H;
    table[SDL_SCANCODE_I]           = KEY_I;
    table[SDL_SCANCODE_J]           = KEY_J;
    table[SDL_SCANCODE_K]           = KEY_K;
    table[SDL_SCANCODE_L]           = KEY_L;
    table[SDL_SCANCODE_M]           = KEY_M;
    table[SDL_SCANCODE_N]           = KEY_N;
    table[SDL_SCANCODE_O]           = KEY_O;
    table[SDL_SCANCODE_P]           = KEY_P;
    table[SDL_SCANCODE_Q]           = KEY_Q;
    table[SDL_SCANCODE_R]           = KEY_R;
    table[SDL_SCANCODE_S]           = KEY_S;
    table[SDL_SCANCODE_T]           = KEY_T;
    table[SDL_SCANCODE_U]           = KEY_U;
    table[SDL_SCANCODE_V]           = KEY_V;
    table[SDL_SCANCODE_W]           = KEY_W;
    table[SDL_SCANCODE_X]           = KEY_X;
    table[SDL_SCANCODE_Y]           = KEY_Y;
    table[SDL_SCANCODE_Z]           = KEY_Z;
    table[SDL_SCANCODE_0]           = KEY_0;
    table[SDL_SCANCODE_1]           = KEY_1;
    table[SDL_SCANCODE_2]           = KEY_2;
    table[SDL_SCANCODE_3]           = KEY_3;
    table[SDL_SCANCODE_4]           = KEY_4;
    table[SDL_SCANCODE_5]           = KEY_5;
    table[SDL_SCANCODE_6]           = KEY_6;
    table[SDL_SCANCODE_7]           = KEY_7;
    table[SDL_SCANCODE_8]           = KEY_8;
    table[SDL_SCANCODE_9]           = KEY_9;
    table[SDL_SCANCODE_SPACE]       = KEY_SPACE;
    table[SDL_SCANCODE_RETURN]      = KEY_ENTER;
    table[SDL_SCANCODE_TAB]         = KEY_TAB;
    table[SDL_SCANCODE_BACKSPACE]   = KEY_BACKSPACE;
    table[SDL_SCANCODE_ESCAPE]      = KEY_ESCAPE;
    table[SDL_SCANCODE_LSHIFT]      = KEY_LEFT_SHIFT;
    table[SDL_SCANCODE_RSHIFT]      = KEY_RIGHT_SHIFT;
    table[SDL_SCANCODE_LCTRL]       = KEY_LEFT_CTRL;
    table[SDL_SCANCODE_RCTRL]       = KEY_RIGHT_CTRL;
    table[SDL_SCANCODE_LALT]        = KEY_LEFT_ALT;
    table[SDL_SCANCODE_RALT]        = KEY_RIGHT_ALT;
    table[SDL_SCANCODE_UP]          = KEY_UP;
    table[SDL_SCANCODE_DOWN]        = KEY_DOWN;
    table[SDL_SCANCODE_LEFT]        = KEY_LEFT;
    table[SDL_SCANCODE_RIGHT]       = KEY_RIGHT;
    table[SDL_SCANCODE_F1]          = KEY_F1;
    table[SDL_SCANCODE_F2]          = KEY_F2;
    table[SDL_SCANCODE_F3]          = KEY_F3;
    table[SDL_SCANCODE_F4]          = KEY_F4;
    table[SDL_SCANCODE_F5]          = KEY_F5;
    table[SDL_SCANCODE_F6]          = KEY_F6;
    table[SDL_SCANCODE_F7]          = KEY_F7;
    table[SDL_SCANCODE_F8]          = KEY_F8;
    table[SDL_SCANCODE_F9]          = KEY_F9;
    table[SDL_SCANCODE_F10]         = KEY_F10;
    table[SDL_SCANCODE_F11]         = KEY_F11;
    table[SDL_SCANCODE_F12]         = KEY_F12;
}

static void load_InputCode_table(SDL_Scancode* table)
{
    for (int i = 0; i < INPUT_CODE_COUNT; ++i)
        table[i] = SDL_SCANCODE_UNKNOWN;

    table[KEY_A] = SDL_SCANCODE_A;
    table[KEY_B] = SDL_SCANCODE_B;
    table[KEY_C] = SDL_SCANCODE_C;
    table[KEY_D] = SDL_SCANCODE_D;
    table[KEY_E] = SDL_SCANCODE_E;
    table[KEY_F] = SDL_SCANCODE_F;
    table[KEY_G] = SDL_SCANCODE_G;
    table[KEY_H] = SDL_SCANCODE_H;
    table[KEY_I] = SDL_SCANCODE_I;
    table[KEY_J] = SDL_SCANCODE_J;
    table[KEY_K] = SDL_SCANCODE_K;
    table[KEY_L] = SDL_SCANCODE_L;
    table[KEY_M] = SDL_SCANCODE_M;
    table[KEY_N] = SDL_SCANCODE_N;
    table[KEY_O] = SDL_SCANCODE_O;
    table[KEY_P] = SDL_SCANCODE_P;
    table[KEY_Q] = SDL_SCANCODE_Q;
    table[KEY_R] = SDL_SCANCODE_R;
    table[KEY_S] = SDL_SCANCODE_S;
    table[KEY_T] = SDL_SCANCODE_T;
    table[KEY_U] = SDL_SCANCODE_U;
    table[KEY_V] = SDL_SCANCODE_V;
    table[KEY_W] = SDL_SCANCODE_W;
    table[KEY_X] = SDL_SCANCODE_X;
    table[KEY_Y] = SDL_SCANCODE_Y;
    table[KEY_Z] = SDL_SCANCODE_Z;
    table[KEY_0] = SDL_SCANCODE_0;
    table[KEY_1] = SDL_SCANCODE_1;
    table[KEY_2] = SDL_SCANCODE_2;
    table[KEY_3] = SDL_SCANCODE_3;
    table[KEY_4] = SDL_SCANCODE_4;
    table[KEY_5] = SDL_SCANCODE_5;
    table[KEY_6] = SDL_SCANCODE_6;
    table[KEY_7] = SDL_SCANCODE_7;
    table[KEY_8] = SDL_SCANCODE_8;
    table[KEY_9] = SDL_SCANCODE_9;
    table[KEY_SPACE] = SDL_SCANCODE_SPACE;
    table[KEY_ENTER] = SDL_SCANCODE_RETURN;
    table[KEY_TAB] = SDL_SCANCODE_TAB;
    table[KEY_BACKSPACE] = SDL_SCANCODE_BACKSPACE;
    table[KEY_ESCAPE] = SDL_SCANCODE_ESCAPE;
    table[KEY_LEFT_SHIFT] = SDL_SCANCODE_LSHIFT;
    table[KEY_RIGHT_SHIFT] = SDL_SCANCODE_RSHIFT;
    table[KEY_LEFT_CTRL] = SDL_SCANCODE_LCTRL;
    table[KEY_RIGHT_CTRL] = SDL_SCANCODE_RCTRL;
    table[KEY_LEFT_ALT] = SDL_SCANCODE_LALT;
    table[KEY_RIGHT_ALT] = SDL_SCANCODE_RALT;
    table[KEY_UP] = SDL_SCANCODE_UP;
    table[KEY_DOWN] = SDL_SCANCODE_DOWN;
    table[KEY_LEFT] = SDL_SCANCODE_LEFT;
    table[KEY_RIGHT] = SDL_SCANCODE_RIGHT;
    table[KEY_F1] = SDL_SCANCODE_F1;
    table[KEY_F2] = SDL_SCANCODE_F2;
    table[KEY_F3] = SDL_SCANCODE_F3;
    table[KEY_F4] = SDL_SCANCODE_F4;
    table[KEY_F5] = SDL_SCANCODE_F5;
    table[KEY_F6] = SDL_SCANCODE_F6;
    table[KEY_F7] = SDL_SCANCODE_F7;
    table[KEY_F8] = SDL_SCANCODE_F8;
    table[KEY_F9] = SDL_SCANCODE_F9;
    table[KEY_F10] = SDL_SCANCODE_F10;
    table[KEY_F11] = SDL_SCANCODE_F11;
    table[KEY_F12] = SDL_SCANCODE_F12;
}

InputCode ScanCodeToInputCode(SDL_Scancode scancode)
{
    assert(scancode >=0 && scancode < SDL_SCANCODE_COUNT);
    return g_scancode_to_input_code[scancode];
}

SDL_Scancode InputCodeToScanCode(InputCode code)
{
    assert(code >= 0 && code < INPUT_CODE_COUNT);
    return g_input_code_to_scancode[static_cast<int>(code)];
}

InputCode InputCodeFromMouseButton(int button)
{
    switch (button)
    {
    case SDL_BUTTON_LEFT: return MOUSE_LEFT;
    case SDL_BUTTON_RIGHT: return MOUSE_RIGHT;
    case SDL_BUTTON_MIDDLE: return MOUSE_MIDDLE;
    case SDL_BUTTON_X1: return MOUSE_BUTTON_4;
    case SDL_BUTTON_X2: return MOUSE_BUTTON_5;
    default: return INPUT_CODE_COUNT;
    }
}
#endif

void InitInputCodes()
{
    // InitScanCodeTable(g_scancode_to_input_code);
    // load_InputCode_table(g_input_code_to_scancode);
}

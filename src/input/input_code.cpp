//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

namespace noz {

    const char* g_input_code_strings[INPUT_CODE_COUNT] = {};

    void InitStrings() {
        for (int i=0; i<INPUT_CODE_COUNT; i++)
            g_input_code_strings[i] = "?";

        g_input_code_strings[KEY_A] = "A";
        g_input_code_strings[KEY_B] = "B";
        g_input_code_strings[KEY_C] = "C";
        g_input_code_strings[KEY_D] = "D";
        g_input_code_strings[KEY_E] = "E";
        g_input_code_strings[KEY_F] = "F";
        g_input_code_strings[KEY_G] = "G";
        g_input_code_strings[KEY_H] = "H";
        g_input_code_strings[KEY_I] = "I";
        g_input_code_strings[KEY_J] = "J";
        g_input_code_strings[KEY_K] = "K";
        g_input_code_strings[KEY_L] = "L";
        g_input_code_strings[KEY_M] = "M";
        g_input_code_strings[KEY_N] = "N";
        g_input_code_strings[KEY_O] = "O";
        g_input_code_strings[KEY_P] = "P";
        g_input_code_strings[KEY_Q] = "Q";
        g_input_code_strings[KEY_R] = "R";
        g_input_code_strings[KEY_S] = "S";
        g_input_code_strings[KEY_T] = "T";
        g_input_code_strings[KEY_U] = "U";
        g_input_code_strings[KEY_V] = "V";
        g_input_code_strings[KEY_W] = "W";
        g_input_code_strings[KEY_X] = "X";
        g_input_code_strings[KEY_Y] = "Y";
        g_input_code_strings[KEY_Z] = "Z";

        g_input_code_strings[KEY_1] = "1";
        g_input_code_strings[KEY_2] = "2";
        g_input_code_strings[KEY_3] = "3";
        g_input_code_strings[KEY_4] = "4";
        g_input_code_strings[KEY_5] = "5";
        g_input_code_strings[KEY_6] = "6";
        g_input_code_strings[KEY_7] = "7";
        g_input_code_strings[KEY_8] = "8";
        g_input_code_strings[KEY_9] = "9";
        g_input_code_strings[KEY_0] = "0";

        g_input_code_strings[KEY_F1] = "F1";
        g_input_code_strings[KEY_F2] = "F2";
        g_input_code_strings[KEY_F3] = "F3";
        g_input_code_strings[KEY_F4] = "F4";
        g_input_code_strings[KEY_F5] = "F5";
        g_input_code_strings[KEY_F6] = "F6";
        g_input_code_strings[KEY_F7] = "F7";
        g_input_code_strings[KEY_F8] = "F8";
        g_input_code_strings[KEY_F9] = "F9";
        g_input_code_strings[KEY_F10] = "F10";
        g_input_code_strings[KEY_F11] = "F11";
        g_input_code_strings[KEY_F12] = "F12";

        g_input_code_strings[KEY_ESCAPE] = "ESC";
        g_input_code_strings[KEY_TAB] = "TAB";
        g_input_code_strings[KEY_QUOTE] = "\"";

        g_input_code_strings[KEY_LEFT_CTRL] = "CTRL";
        g_input_code_strings[KEY_RIGHT_CTRL] = "CTRL";
        g_input_code_strings[KEY_LEFT_ALT] = "ALT";
        g_input_code_strings[KEY_RIGHT_ALT] = "ALT";
        g_input_code_strings[KEY_LEFT_SHIFT] = "SHIFT";
        g_input_code_strings[KEY_RIGHT_SHIFT] = "SHIFT";
        g_input_code_strings[KEY_LEFT_SUPER] = "WIN";
        g_input_code_strings[KEY_RIGHT_SUPER] = "WIN";

        g_input_code_strings[KEY_LEFT_BRACKET] = "[";
        g_input_code_strings[KEY_RIGHT_BRACKET] = "]";
    }
}
#pragma once

namespace noz
{
    // Input binding types
    enum class InputBindingType
    {
        Keyboard,
        MouseButton,
        GamepadButton
    };

    // Input binding structure
    struct InputBinding
    {
        InputBindingType type;
        SDL_Scancode keyCode;        // For keyboard
        int mouseButton;             // For mouse
        int gamepadButton;           // For gamepad

        InputBinding() 
            : type(InputBindingType::Keyboard)
            , keyCode(SDL_SCANCODE_UNKNOWN)
            , mouseButton(0)
            , gamepadButton(0)
        {
        }

        InputBinding(SDL_Scancode key) 
            : type(InputBindingType::Keyboard)
            , keyCode(key)
            , mouseButton(0)
            , gamepadButton(0)
        {
        }

        InputBinding(InputBindingType bindingType, int button) 
            : type(bindingType)
            , keyCode(SDL_SCANCODE_UNKNOWN)
            , mouseButton(0)
            , gamepadButton(0)
        {
            if (bindingType == InputBindingType::MouseButton)
            {
                mouseButton = button;
            }
            else if (bindingType == InputBindingType::GamepadButton)
            {
                gamepadButton = button;
            }
        }
    };

} // namespace noz 
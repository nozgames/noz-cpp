/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    InputActionMap::InputActionMap(const std::string& name)
        : _name(name)
    {
    }

    InputActionMap::~InputActionMap()
    {
        clearBindings();
    }

    void InputActionMap::addBinding(const InputBinding& binding, std::shared_ptr<InputAction> action)
    {
        if (!action)
            return;

        switch (binding.type)
        {
        case InputBindingType::Keyboard:
            addKeyboardBinding(binding.keyCode, action);
            break;
        case InputBindingType::MouseButton:
            addMouseBinding(binding.mouseButton, action);
            break;
        case InputBindingType::GamepadButton:
            addGamepadBinding(binding.gamepadButton, action);
            break;
        }
    }

    void InputActionMap::addKeyboardBinding(SDL_Scancode keyCode, std::shared_ptr<InputAction> action)
    {
        if (!action)
            return;

        _keyboardBindings[keyCode] = action;
        addActionToList(action);
    }

    void InputActionMap::addMouseBinding(int mouseButton, std::shared_ptr<InputAction> action)
    {
        if (!action)
            return;

        _mouseBindings[mouseButton] = action;
        addActionToList(action);
    }

    void InputActionMap::addGamepadBinding(int gamepadButton, std::shared_ptr<InputAction> action)
    {
        if (!action)
            return;

        _gamepadBindings[gamepadButton] = action;
        addActionToList(action);
    }

    void InputActionMap::removeBinding(const InputBinding& binding)
    {
        switch (binding.type)
        {
        case InputBindingType::Keyboard:
            removeKeyboardBinding(binding.keyCode);
            break;
        case InputBindingType::MouseButton:
            removeMouseBinding(binding.mouseButton);
            break;
        case InputBindingType::GamepadButton:
            removeGamepadBinding(binding.gamepadButton);
            break;
        }
    }

    void InputActionMap::removeKeyboardBinding(SDL_Scancode keyCode)
    {
        _keyboardBindings.erase(keyCode);
    }

    void InputActionMap::removeMouseBinding(int mouseButton)
    {
        _mouseBindings.erase(mouseButton);
    }

    void InputActionMap::removeGamepadBinding(int gamepadButton)
    {
        _gamepadBindings.erase(gamepadButton);
    }

    void InputActionMap::clearBindings()
    {
        _keyboardBindings.clear();
        _mouseBindings.clear();
        _gamepadBindings.clear();
        _actions.clear();
    }

    std::shared_ptr<InputAction> InputActionMap::getActionForKey(SDL_Scancode keyCode) const
    {
        auto it = _keyboardBindings.find(keyCode);
        return (it != _keyboardBindings.end()) ? it->second : nullptr;
    }

    std::shared_ptr<InputAction> InputActionMap::getActionForMouseButton(int mouseButton) const
    {
        auto it = _mouseBindings.find(mouseButton);
        return (it != _mouseBindings.end()) ? it->second : nullptr;
    }

    std::shared_ptr<InputAction> InputActionMap::getActionForGamepadButton(int gamepadButton) const
    {
        auto it = _gamepadBindings.find(gamepadButton);
        return (it != _gamepadBindings.end()) ? it->second : nullptr;
    }

    void InputActionMap::addActionToList(std::shared_ptr<InputAction> action)
    {
        if (!action)
            return;

        // Check if action is already in the list
        auto it = std::find(_actions.begin(), _actions.end(), action);
        if (it == _actions.end())
        {
            _actions.push_back(action);
        }
    }

} // namespace noz 
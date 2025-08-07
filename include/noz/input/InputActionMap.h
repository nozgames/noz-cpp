/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "InputBinding.h"

namespace noz
{
    class InputAction;

    class InputActionMap
    {
    public:
        InputActionMap(const std::string& name);
        ~InputActionMap();

        // Get the action map name
        const std::string& getName() const { return _name; }

        // Add a binding to an action
        void addBinding(const InputBinding& binding, std::shared_ptr<InputAction> action);
        void addKeyboardBinding(SDL_Scancode keyCode, std::shared_ptr<InputAction> action);
        void addMouseBinding(int mouseButton, std::shared_ptr<InputAction> action);
        void addGamepadBinding(int gamepadButton, std::shared_ptr<InputAction> action);

        // Remove a binding
        void removeBinding(const InputBinding& binding);
        void removeKeyboardBinding(SDL_Scancode keyCode);
        void removeMouseBinding(int mouseButton);
        void removeGamepadBinding(int gamepadButton);

        // Clear all bindings
        void clearBindings();

        // Get action for a specific binding
        std::shared_ptr<InputAction> getActionForKey(SDL_Scancode keyCode) const;
        std::shared_ptr<InputAction> getActionForMouseButton(int mouseButton) const;
        std::shared_ptr<InputAction> getActionForGamepadButton(int gamepadButton) const;

        // Get all actions in this map
        const std::vector<std::shared_ptr<InputAction>>& getActions() const { return _actions; }

    private:
        std::string _name;
        std::unordered_map<SDL_Scancode, std::shared_ptr<InputAction>> _keyboardBindings;
        std::unordered_map<int, std::shared_ptr<InputAction>> _mouseBindings;
        std::unordered_map<int, std::shared_ptr<InputAction>> _gamepadBindings;
        std::vector<std::shared_ptr<InputAction>> _actions;

        // Helper to add action to the actions list if not already present
        void addActionToList(std::shared_ptr<InputAction> action);
    };

} // namespace noz 
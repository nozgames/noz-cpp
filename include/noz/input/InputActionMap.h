/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "InputCode.h"

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

        // Unified binding system - much simpler!
        void bindAction(InputCode inputCode, std::shared_ptr<InputAction> action);
        void unbindAction(InputCode inputCode);
        
        // Convenience methods for creating actions
        std::shared_ptr<InputAction> createAction(const std::string& actionName, InputActionType type = InputActionType::Button);
        std::shared_ptr<InputAction> createButtonAction(const std::string& actionName);
        std::shared_ptr<InputAction> createValueAction(const std::string& actionName);

        // Clear all bindings
        void clearBindings();

        // Get action bound to input code
        std::shared_ptr<InputAction> getAction(InputCode inputCode) const;
        
        // Get action by name
        std::shared_ptr<InputAction> getActionByName(const std::string& name) const;

        // Get all actions in this map
        const std::vector<std::shared_ptr<InputAction>>& getActions() const { return _actions; }
        
        // Enable/disable all actions in this map
        void enable();
        void disable();
        bool isEnabled() const { return _enabled; }

    private:
        std::string _name;
        bool _enabled;
        
        // Single binding map - much cleaner!
        std::unordered_map<InputCode, std::shared_ptr<InputAction>> _bindings;
        std::vector<std::shared_ptr<InputAction>> _actions;

        // Helper to add action to the actions list if not already present
        void addActionToList(std::shared_ptr<InputAction> action);
    };

} // namespace noz 
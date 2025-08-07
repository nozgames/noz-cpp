#pragma once

namespace noz
{
    // Input action callbacks
    using InputActionCallback = std::function<void()>;

    class InputAction
    {
    public:
        InputAction(const std::string& name);
        ~InputAction();

        // Set callbacks
        void setBeginCallback(InputActionCallback callback) { _beginCallback = callback; }
        void setEndCallback(InputActionCallback callback) { _endCallback = callback; }

        // Event-driven state management (called by InputSystem)
        void onPressed();
        void onReleased();

        // Check if action is currently active
        bool isActive() const { return _isActive; }

        // Get action name
        const std::string& getName() const { return _name; }

    private:
        std::string _name;
        InputActionCallback _beginCallback;
        InputActionCallback _endCallback;
        bool _isActive;
        bool _wasActive;

        // Update action state and trigger callbacks
        void updateState(bool newActive);
    };

} // namespace noz 
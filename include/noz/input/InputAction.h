#pragma once

#include "InputCode.h"

namespace noz
{
    // Forward declarations
    struct InputActionCallbackContext;
    
    // Input action callbacks (Unity-style with context)
    using InputActionCallback = std::function<void(const InputActionCallbackContext& context)>;
    
    // Action types (like Unity's InputActionType)
    enum class InputActionType
    {
        Button,    // Digital on/off input (keyboard keys, mouse buttons, gamepad buttons)
        Value      // Analog input (mouse axes, gamepad sticks/triggers)
    };
    
    // Action phase (like Unity's InputActionPhase)
    enum class InputActionPhase
    {
        Disabled,  // Action is disabled
        Waiting,   // Waiting for input
        Started,   // Input just started
        Performed, // Input is being performed (held for buttons, continuous for values)
        Canceled   // Input was canceled
    };
    
    // Context provided to callbacks (like Unity's InputAction.CallbackContext)
    struct InputActionCallbackContext
    {
        InputActionPhase phase;
        InputActionType actionType;
        float value;           // For analog inputs (0-1 for buttons, -1 to 1 for axes)
        glm::vec2 value2D;     // For 2D inputs like joysticks
        float deltaTime;       // Time since last update
        bool performed;        // True if action was just performed
        bool started;          // True if action just started
        bool canceled;         // True if action was canceled
        
        // Convenience methods (like Unity)
        bool ReadButtonPressed() const { return phase == InputActionPhase::Started; }
        bool ReadButton() const { return phase == InputActionPhase::Performed; }
        bool ReadButtonReleased() const { return phase == InputActionPhase::Canceled; }
        float ReadValue() const { return value; }
        glm::vec2 ReadValue2D() const { return value2D; }
    };

    class InputAction
    {
    public:
        InputAction(const std::string& name, InputActionType type = InputActionType::Button);
        ~InputAction();

        // Unity-style callback management
        void setCallback(InputActionCallback callback) { _callback = callback; }
        
        // Enable/disable action
        void enable() { _enabled = true; }
        void disable() { _enabled = false; }
        bool isEnabled() const { return _enabled; }

        // Get current state (like Unity's ReadValue methods)
        float readValue() const { return _currentValue; }
        glm::vec2 readValue2D() const { return _currentValue2D; }
        bool readButtonPressed() const { return _phase == InputActionPhase::Started; }
        bool readButton() const { return _phase == InputActionPhase::Performed; }
        bool readButtonReleased() const { return _phase == InputActionPhase::Canceled; }
        
        // Get current phase
        InputActionPhase getPhase() const { return _phase; }
        InputActionType getType() const { return _type; }
        
        // Get action name
        const std::string& getName() const { return _name; }
        
        // Internal methods (called by InputSystem)
        void update(float value, float deltaTime);
        void update2D(const glm::vec2& value, float deltaTime);
        void reset();

    private:
        std::string _name;
        InputActionType _type;
        InputActionCallback _callback;
        
        bool _enabled;
        InputActionPhase _phase;
        InputActionPhase _previousPhase;
        
        float _currentValue;
        float _previousValue;
        glm::vec2 _currentValue2D;
        glm::vec2 _previousValue2D;
        
        float _buttonPressThreshold;  // Threshold for considering axis input as button press
        
        void updatePhase(bool isPressed, float deltaTime);
        void triggerCallback(InputActionPhase phase, float deltaTime);
    };

} // namespace noz 
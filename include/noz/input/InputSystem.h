/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include "InputCode.h"

namespace noz
{
    class InputAction;
    class InputActionMap;
    
    // Forward declare internal class
    class Controller;

    // Input event types
    enum class InputEventType
    {
        KeyDown,
        KeyUp,
        MouseMove,
        MouseButtonDown,
        MouseButtonUp,
        MouseWheel,
        ControllerButtonDown,
        ControllerButtonUp,
        ControllerAxisMotion,
        ControllerConnected,
        ControllerDisconnected
    };

    // Input event data structure
    struct InputEvent
    {
        InputEventType type;
        SDL_Scancode keyCode;
        float mouseX;
        float mouseY;
        int mouseButton;
        float mouseWheelX;
        float mouseWheelY;
        bool isPressed;
        
        // Controller-specific data
        int controllerIndex;     // Which controller (0-3)
        int controllerButton;    // Button index
        int controllerAxis;      // Axis index
        float axisValue;         // Axis value (-1 to 1 for sticks, 0 to 1 for triggers)
    };

    // Input handler callback type
    using InputHandler = std::function<void(const InputEvent&)>;

    class InputSystem : public ISingleton<InputSystem>
    {
    public:
        InputSystem();
        ~InputSystem();

        // Register input handlers
        void RegisterHandler(InputEventType eventType, InputHandler handler);

        // Process SDL events (called from Application::Update)
        void ProcessEvents(SDL_Event& event);
        
        // Update controllers (called each frame after event processing)
        void updateControllers();

        // Check if a key is currently pressed
        bool IsKeyPressed(SDL_Scancode keyCode) const;
        
        // Check if a mouse button is currently pressed
        bool IsMouseButtonPressed(int button) const;
        
        // Get current mouse position
        void GetMousePosition(float& x, float& y) const;
        
        // Controller support (simplified - use InputActions instead)
        int GetConnectedControllerCount() const;
        bool IsControllerConnected(int playerIndex) const;

        // Action map management
        void setActiveActionMap(InputActionMap* actionMap);
        InputActionMap* getActiveActionMap() const { return _activeActionMap; }

		static void load();
		static void unload();

    private:

        friend class ISingleton<InputSystem>;

		void loadInternal();
		void unloadInternal();
		void handleControllerAdded(SDL_JoystickID joystickId);
		void handleControllerRemoved(SDL_JoystickID joystickId);
		int findAvailableControllerSlot() const;
		
		// Conversion functions
		InputCode sdlScancodeToInputCode(SDL_Scancode scancode) const;
		InputCode sdlMouseButtonToInputCode(int button) const;
		void updateInputActions(float deltaTime);
        
        std::unordered_map<InputEventType, std::vector<InputHandler>> _handlers;
        std::unordered_map<SDL_Scancode, bool> _keyStates;
        std::unordered_map<int, bool> _mouseButtonStates;
        
        InputActionMap* _activeActionMap;
        
        float _mouseX;
        float _mouseY;
        
        // Controller management (integrated)
        static constexpr int MaxControllers = 4;
        std::array<std::unique_ptr<Controller>, MaxControllers> _controllers;
        std::unordered_map<SDL_JoystickID, int> _joystickToController;
    };
} // namespace noz 
/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz
{
    class InputAction;
    class InputActionMap;

    // Input event types
    enum class InputEventType
    {
        KeyDown,
        KeyUp,
        MouseMove,
        MouseButtonDown,
        MouseButtonUp,
        MouseWheel
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

        // Check if a key is currently pressed
        bool IsKeyPressed(SDL_Scancode keyCode) const;
        
        // Check if a mouse button is currently pressed
        bool IsMouseButtonPressed(int button) const;
        
        // Get current mouse position
        void GetMousePosition(float& x, float& y) const;

        // Action map management
        void setActiveActionMap(InputActionMap* actionMap);
        InputActionMap* getActiveActionMap() const { return _activeActionMap; }

		static void load();
		static void unload();

    private:

        friend class ISingleton<InputSystem>;

		void loadInternal();
		void unloadInternal();
        
        std::unordered_map<InputEventType, std::vector<InputHandler>> _handlers;
        std::unordered_map<SDL_Scancode, bool> _keyStates;
        std::unordered_map<int, bool> _mouseButtonStates;
        
        InputActionMap* _activeActionMap;
        
        float _mouseX;
        float _mouseY;
    };
} // namespace noz 
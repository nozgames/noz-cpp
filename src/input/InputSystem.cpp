/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    InputSystem::InputSystem()
        : _activeActionMap(nullptr)
        , _mouseX(0.0f)
        , _mouseY(0.0f)
    {
    }

    InputSystem::~InputSystem()
    {
        // onUnload will be called by ISingleton::Unload()
    }

    void InputSystem::RegisterHandler(InputEventType eventType, InputHandler handler)
    {
        _handlers[eventType].push_back(handler);
    }

    void InputSystem::ProcessEvents(SDL_Event& event)
    {
        InputEvent inputEvent = {};
        
        switch (event.type)
        {
        case SDL_EVENT_KEY_DOWN:
            inputEvent.type = InputEventType::KeyDown;
            inputEvent.keyCode = event.key.scancode;
            inputEvent.isPressed = true;
            _keyStates[inputEvent.keyCode] = true;
            
            // Trigger action from active action map
            if (_activeActionMap)
            {
                std::shared_ptr<InputAction> action = _activeActionMap->getActionForKey(inputEvent.keyCode);
                if (action)
                {
                    action->onPressed();
                }
            }
            break;

        case SDL_EVENT_KEY_UP:
            inputEvent.type = InputEventType::KeyUp;
            inputEvent.keyCode = event.key.scancode;
            inputEvent.isPressed = false;
            _keyStates[inputEvent.keyCode] = false;
            
            // Trigger action from active action map
            if (_activeActionMap)
            {
                std::shared_ptr<InputAction> action = _activeActionMap->getActionForKey(inputEvent.keyCode);
                if (action)
                {
                    action->onReleased();
                }
            }
            break;

        case SDL_EVENT_MOUSE_MOTION:
            inputEvent.type = InputEventType::MouseMove;
            inputEvent.mouseX = static_cast<float>(event.motion.x);
            inputEvent.mouseY = static_cast<float>(event.motion.y);
            _mouseX = inputEvent.mouseX;
            _mouseY = inputEvent.mouseY;
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            inputEvent.type = InputEventType::MouseButtonDown;
            inputEvent.mouseX = static_cast<float>(event.button.x);
            inputEvent.mouseY = static_cast<float>(event.button.y);
            inputEvent.mouseButton = event.button.button;
            inputEvent.isPressed = true;
            _mouseX = inputEvent.mouseX;
            _mouseY = inputEvent.mouseY;
            _mouseButtonStates[inputEvent.mouseButton] = true;
            
            // Trigger action from active action map
            if (_activeActionMap)
            {
                std::shared_ptr<InputAction> action = _activeActionMap->getActionForMouseButton(inputEvent.mouseButton);
                if (action)
                {
                    action->onPressed();
                }
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            inputEvent.type = InputEventType::MouseButtonUp;
            inputEvent.mouseX = static_cast<float>(event.button.x);
            inputEvent.mouseY = static_cast<float>(event.button.y);
            inputEvent.mouseButton = event.button.button;
            inputEvent.isPressed = false;
            _mouseX = inputEvent.mouseX;
            _mouseY = inputEvent.mouseY;
            _mouseButtonStates[inputEvent.mouseButton] = false;
            
            // Trigger action from active action map
            if (_activeActionMap)
            {
                std::shared_ptr<InputAction> action = _activeActionMap->getActionForMouseButton(inputEvent.mouseButton);
                if (action)
                {
                    action->onReleased();
                }
            }
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            inputEvent.type = InputEventType::MouseWheel;
            inputEvent.mouseWheelX = static_cast<float>(event.wheel.x);
            inputEvent.mouseWheelY = static_cast<float>(event.wheel.y);
            // Include current mouse position in wheel events
            inputEvent.mouseX = _mouseX;
            inputEvent.mouseY = _mouseY;
            break;

        default:
            return; // Unhandled event type
        }

        // Call all registered handlers for this event type
        auto it = _handlers.find(inputEvent.type);
        if (it != _handlers.end())
        {
            for (const auto& handler : it->second)
            {
                handler(inputEvent);
            }
        }
    }

    bool InputSystem::IsKeyPressed(SDL_Scancode keyCode) const
    {
        auto it = _keyStates.find(keyCode);
        return it != _keyStates.end() && it->second;
    }

    bool InputSystem::IsMouseButtonPressed(int button) const
    {
        auto it = _mouseButtonStates.find(button);
        return it != _mouseButtonStates.end() && it->second;
    }

    void InputSystem::GetMousePosition(float& x, float& y) const
    {
        x = _mouseX;
        y = _mouseY;
    }

    void InputSystem::setActiveActionMap(InputActionMap* actionMap)
    {
        _activeActionMap = actionMap;
    }

	void InputSystem::load()
	{
		ISingleton<InputSystem>::load();
		instance()->loadInternal();
	}

    void InputSystem::loadInternal()
    {
        // Initialize mouse position - SDL3 returns floats
        float mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        _mouseX = mouseX;
        _mouseY = mouseY;
    }

	void InputSystem::unload()
	{
		instance()->unloadInternal();
		ISingleton<InputSystem>::unload();
	}

    void InputSystem::unloadInternal()
    {
        _handlers.clear();
        _keyStates.clear();
        _mouseButtonStates.clear();
        _activeActionMap = nullptr;
    }
}

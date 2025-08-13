/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "noz/input/InputSystem.h"
#include "internal/Controller.h"
#include "noz/input/InputAction.h"
#include "noz/input/InputActionMap.h"
#include "noz/input/InputCode.h"

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
            break;

        case SDL_EVENT_KEY_UP:
            inputEvent.type = InputEventType::KeyUp;
            inputEvent.keyCode = event.key.scancode;
            inputEvent.isPressed = false;
            _keyStates[inputEvent.keyCode] = false;
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
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            inputEvent.type = InputEventType::MouseWheel;
            inputEvent.mouseWheelX = static_cast<float>(event.wheel.x);
            inputEvent.mouseWheelY = static_cast<float>(event.wheel.y);
            // Include current mouse position in wheel events
            inputEvent.mouseX = _mouseX;
            inputEvent.mouseY = _mouseY;
            break;
            
        case SDL_EVENT_GAMEPAD_ADDED:
            handleControllerAdded(event.gdevice.which);
            return; // Handled internally
            
        case SDL_EVENT_GAMEPAD_REMOVED:
            handleControllerRemoved(event.gdevice.which);
            return; // Handled internally

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
    
    // Controller methods removed from public API - use InputActions instead
    
    int InputSystem::GetConnectedControllerCount() const
    {
        int count = 0;
        for (const auto& controller : _controllers)
        {
            if (controller && controller->isConnected())
                count++;
        }
        return count;
    }
    
    bool InputSystem::IsControllerConnected(int playerIndex) const
    {
        if (playerIndex < 0 || playerIndex >= MaxControllers)
            return false;
        return _controllers[playerIndex] && _controllers[playerIndex]->isConnected();
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
        
        // Initialize controller support
        SDL_UpdateGamepads();
        
        // Check for already connected controllers
        int numJoysticks = 0;
        SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
        
        if (joysticks)
        {
            for (int i = 0; i < numJoysticks && i < MaxControllers; i++)
            {
                if (SDL_IsGamepad(joysticks[i]))
                {
                    handleControllerAdded(joysticks[i]);
                }
            }
            SDL_free(joysticks);
        }
        
        Log::info("InputSystem", "Controller support initialized. " + std::to_string(GetConnectedControllerCount()) + " controller(s) connected.");
    }

	void InputSystem::unload()
	{
		instance()->unloadInternal();
		ISingleton<InputSystem>::unload();
	}

    void InputSystem::unloadInternal()
    {
        // Close all controllers
        for (auto& controller : _controllers)
        {
            controller.reset();
        }
        _joystickToController.clear();
        
        _handlers.clear();
        _keyStates.clear();
        _mouseButtonStates.clear();
        _activeActionMap = nullptr;
    }
    
    void InputSystem::updateControllers()
    {
        // Update all connected controllers
        for (auto& controller : _controllers)
        {
            if (controller && controller->isConnected())
            {
                controller->update();
            }
        }
        
        // Update input actions with current frame data
        updateInputActions(1.0f / 60.0f); // TODO: Get real deltaTime
    }
    
    void InputSystem::handleControllerAdded(SDL_JoystickID joystickId)
    {
        // Check if already assigned
        if (_joystickToController.find(joystickId) != _joystickToController.end())
            return;
        
        // Find available controller slot
        int controllerIndex = findAvailableControllerSlot();
        if (controllerIndex < 0)
        {
            Log::warning("InputSystem", "No available controller slots for new controller");
            return;
        }
        
        // Open the gamepad
        SDL_Gamepad* gamepad = SDL_OpenGamepad(joystickId);
        if (!gamepad)
        {
            Log::error("InputSystem", "Failed to open gamepad");
            return;
        }
        
        // Create controller
        _controllers[controllerIndex] = std::make_unique<Controller>(controllerIndex, gamepad);
        _joystickToController[joystickId] = controllerIndex;
        
        // Send connected event
        InputEvent event = {};
        event.type = InputEventType::ControllerConnected;
        event.controllerIndex = controllerIndex;
        
        auto it = _handlers.find(InputEventType::ControllerConnected);
        if (it != _handlers.end())
        {
            for (const auto& handler : it->second)
            {
                handler(event);
            }
        }
        
        Log::info("InputSystem", "Controller connected to slot " + std::to_string(controllerIndex));
    }
    
    void InputSystem::handleControllerRemoved(SDL_JoystickID joystickId)
    {
        auto it = _joystickToController.find(joystickId);
        if (it == _joystickToController.end())
            return;
        
        int controllerIndex = it->second;
        
        // Send disconnected event
        InputEvent event = {};
        event.type = InputEventType::ControllerDisconnected;
        event.controllerIndex = controllerIndex;
        
        auto handlers = _handlers.find(InputEventType::ControllerDisconnected);
        if (handlers != _handlers.end())
        {
            for (const auto& handler : handlers->second)
            {
                handler(event);
            }
        }
        
        if (_controllers[controllerIndex])
        {
            _controllers[controllerIndex]->handleDisconnect();
        }
        
        _joystickToController.erase(it);
        
        Log::info("InputSystem", "Controller removed from slot " + std::to_string(controllerIndex));
    }
    
    int InputSystem::findAvailableControllerSlot() const
    {
        for (int i = 0; i < MaxControllers; i++)
        {
            if (!_controllers[i] || !_controllers[i]->isConnected())
            {
                return i;
            }
        }
        return -1; // No available slots
    }
    
    InputCode InputSystem::sdlScancodeToInputCode(SDL_Scancode scancode) const
    {
        switch (scancode)
        {
            case SDL_SCANCODE_A: return InputCode::KeyA;
            case SDL_SCANCODE_B: return InputCode::KeyB;
            case SDL_SCANCODE_C: return InputCode::KeyC;
            case SDL_SCANCODE_D: return InputCode::KeyD;
            case SDL_SCANCODE_E: return InputCode::KeyE;
            case SDL_SCANCODE_F: return InputCode::KeyF;
            case SDL_SCANCODE_G: return InputCode::KeyG;
            case SDL_SCANCODE_H: return InputCode::KeyH;
            case SDL_SCANCODE_I: return InputCode::KeyI;
            case SDL_SCANCODE_J: return InputCode::KeyJ;
            case SDL_SCANCODE_K: return InputCode::KeyK;
            case SDL_SCANCODE_L: return InputCode::KeyL;
            case SDL_SCANCODE_M: return InputCode::KeyM;
            case SDL_SCANCODE_N: return InputCode::KeyN;
            case SDL_SCANCODE_O: return InputCode::KeyO;
            case SDL_SCANCODE_P: return InputCode::KeyP;
            case SDL_SCANCODE_Q: return InputCode::KeyQ;
            case SDL_SCANCODE_R: return InputCode::KeyR;
            case SDL_SCANCODE_S: return InputCode::KeyS;
            case SDL_SCANCODE_T: return InputCode::KeyT;
            case SDL_SCANCODE_U: return InputCode::KeyU;
            case SDL_SCANCODE_V: return InputCode::KeyV;
            case SDL_SCANCODE_W: return InputCode::KeyW;
            case SDL_SCANCODE_X: return InputCode::KeyX;
            case SDL_SCANCODE_Y: return InputCode::KeyY;
            case SDL_SCANCODE_Z: return InputCode::KeyZ;
            
            case SDL_SCANCODE_0: return InputCode::Key0;
            case SDL_SCANCODE_1: return InputCode::Key1;
            case SDL_SCANCODE_2: return InputCode::Key2;
            case SDL_SCANCODE_3: return InputCode::Key3;
            case SDL_SCANCODE_4: return InputCode::Key4;
            case SDL_SCANCODE_5: return InputCode::Key5;
            case SDL_SCANCODE_6: return InputCode::Key6;
            case SDL_SCANCODE_7: return InputCode::Key7;
            case SDL_SCANCODE_8: return InputCode::Key8;
            case SDL_SCANCODE_9: return InputCode::Key9;
            
            case SDL_SCANCODE_SPACE: return InputCode::KeySpace;
            case SDL_SCANCODE_RETURN: return InputCode::KeyEnter;
            case SDL_SCANCODE_TAB: return InputCode::KeyTab;
            case SDL_SCANCODE_BACKSPACE: return InputCode::KeyBackspace;
            case SDL_SCANCODE_ESCAPE: return InputCode::KeyEscape;
            
            case SDL_SCANCODE_LSHIFT: return InputCode::KeyLeftShift;
            case SDL_SCANCODE_RSHIFT: return InputCode::KeyRightShift;
            case SDL_SCANCODE_LCTRL: return InputCode::KeyLeftCtrl;
            case SDL_SCANCODE_RCTRL: return InputCode::KeyRightCtrl;
            case SDL_SCANCODE_LALT: return InputCode::KeyLeftAlt;
            case SDL_SCANCODE_RALT: return InputCode::KeyRightAlt;
            
            case SDL_SCANCODE_UP: return InputCode::KeyUp;
            case SDL_SCANCODE_DOWN: return InputCode::KeyDown;
            case SDL_SCANCODE_LEFT: return InputCode::KeyLeft;
            case SDL_SCANCODE_RIGHT: return InputCode::KeyRight;
            
            case SDL_SCANCODE_F1: return InputCode::KeyF1;
            case SDL_SCANCODE_F2: return InputCode::KeyF2;
            case SDL_SCANCODE_F3: return InputCode::KeyF3;
            case SDL_SCANCODE_F4: return InputCode::KeyF4;
            case SDL_SCANCODE_F5: return InputCode::KeyF5;
            case SDL_SCANCODE_F6: return InputCode::KeyF6;
            case SDL_SCANCODE_F7: return InputCode::KeyF7;
            case SDL_SCANCODE_F8: return InputCode::KeyF8;
            case SDL_SCANCODE_F9: return InputCode::KeyF9;
            case SDL_SCANCODE_F10: return InputCode::KeyF10;
            case SDL_SCANCODE_F11: return InputCode::KeyF11;
            case SDL_SCANCODE_F12: return InputCode::KeyF12;
            
            default: return InputCode::None;
        }
    }
    
    InputCode InputSystem::sdlMouseButtonToInputCode(int button) const
    {
        switch (button)
        {
            case SDL_BUTTON_LEFT: return InputCode::MouseLeft;
            case SDL_BUTTON_RIGHT: return InputCode::MouseRight;
            case SDL_BUTTON_MIDDLE: return InputCode::MouseMiddle;
            case SDL_BUTTON_X1: return InputCode::MouseButton4;
            case SDL_BUTTON_X2: return InputCode::MouseButton5;
            default: return InputCode::None;
        }
    }
    
    void InputSystem::updateInputActions(float deltaTime)
    {
        if (!_activeActionMap)
            return;
            
        // Update all actions with current input states
        const auto& actions = _activeActionMap->getActions();
        for (const auto& action : actions)
        {
            if (!action->isEnabled())
                continue;
                
            // Check all input codes to see if any are bound to this action
            bool isPressed = false;
            
            // Check keyboard keys
            for (const auto& [keyCode, pressed] : _keyStates)
            {
                InputCode inputCode = sdlScancodeToInputCode(keyCode);
                auto boundAction = _activeActionMap->getAction(inputCode);
                if (boundAction == action && pressed)
                {
                    isPressed = true;
                    break;
                }
            }
            
            // Check mouse buttons if not already pressed
            if (!isPressed)
            {
                for (const auto& [mouseButton, pressed] : _mouseButtonStates)
                {
                    InputCode inputCode = sdlMouseButtonToInputCode(mouseButton);
                    auto boundAction = _activeActionMap->getAction(inputCode);
                    if (boundAction == action && pressed)
                    {
                        isPressed = true;
                        break;
                    }
                }
            }
            
            // Update the action with the current state
            if (action->getType() == InputActionType::Button)
            {
                action->update(isPressed ? 1.0f : 0.0f, deltaTime);
            }
        }
    }
}

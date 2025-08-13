/*
	NoZ Game Engine
	
	Copyright(c) 2025 NoZ Games, LLC
*/

#pragma once

#include <SDL3/SDL_gamepad.h>

namespace noz
{
	// Controller button mappings (matches SDL GameController standard)
	enum class ControllerButton
	{
		Invalid = -1,
		A = SDL_GAMEPAD_BUTTON_SOUTH,         // Cross on PS, A on Xbox
		B = SDL_GAMEPAD_BUTTON_EAST,          // Circle on PS, B on Xbox
		X = SDL_GAMEPAD_BUTTON_WEST,          // Square on PS, X on Xbox
		Y = SDL_GAMEPAD_BUTTON_NORTH,         // Triangle on PS, Y on Xbox
		Back = SDL_GAMEPAD_BUTTON_BACK,
		Guide = SDL_GAMEPAD_BUTTON_GUIDE,
		Start = SDL_GAMEPAD_BUTTON_START,
		LeftStick = SDL_GAMEPAD_BUTTON_LEFT_STICK,
		RightStick = SDL_GAMEPAD_BUTTON_RIGHT_STICK,
		LeftShoulder = SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
		RightShoulder = SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
		DPadUp = SDL_GAMEPAD_BUTTON_DPAD_UP,
		DPadDown = SDL_GAMEPAD_BUTTON_DPAD_DOWN,
		DPadLeft = SDL_GAMEPAD_BUTTON_DPAD_LEFT,
		DPadRight = SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
		Misc1 = SDL_GAMEPAD_BUTTON_MISC1,
		Paddle1 = SDL_GAMEPAD_BUTTON_RIGHT_PADDLE1,
		Paddle2 = SDL_GAMEPAD_BUTTON_LEFT_PADDLE1,
		Paddle3 = SDL_GAMEPAD_BUTTON_RIGHT_PADDLE2,
		Paddle4 = SDL_GAMEPAD_BUTTON_LEFT_PADDLE2,
		Touchpad = SDL_GAMEPAD_BUTTON_TOUCHPAD,
		Max = SDL_GAMEPAD_BUTTON_COUNT
	};
	
	// Controller axis mappings
	enum class ControllerAxis
	{
		Invalid = -1,
		LeftX = SDL_GAMEPAD_AXIS_LEFTX,
		LeftY = SDL_GAMEPAD_AXIS_LEFTY,
		RightX = SDL_GAMEPAD_AXIS_RIGHTX,
		RightY = SDL_GAMEPAD_AXIS_RIGHTY,
		TriggerLeft = SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
		TriggerRight = SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
		Max = SDL_GAMEPAD_AXIS_COUNT
	};
	
	// Controller state
	struct ControllerState
	{
		// Button states
		std::array<bool, static_cast<int>(ControllerButton::Max)> buttons;
		std::array<bool, static_cast<int>(ControllerButton::Max)> buttonsPressed;  // Just pressed this frame
		std::array<bool, static_cast<int>(ControllerButton::Max)> buttonsReleased; // Just released this frame
		
		// Axis values (-1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers)
		std::array<float, static_cast<int>(ControllerAxis::Max)> axes;
		
		// Deadzone values
		float stickDeadzone = 0.15f;
		float triggerDeadzone = 0.1f;
		
		ControllerState()
		{
			buttons.fill(false);
			buttonsPressed.fill(false);
			buttonsReleased.fill(false);
			axes.fill(0.0f);
		}
	};
	
	class Controller
	{
	public:
		Controller(int playerIndex, SDL_Gamepad* gamepad);
		~Controller();
		
		// Update controller state (called each frame)
		void update();
		
		// Query methods
		bool isConnected() const { return _gamepad != nullptr; }
		int getPlayerIndex() const { return _playerIndex; }
		SDL_JoystickID getJoystickId() const { return _joystickId; }
		const std::string& getName() const { return _name; }
		
		// Button queries
		bool isButtonPressed(ControllerButton button) const;
		bool isButtonJustPressed(ControllerButton button) const;
		bool isButtonJustReleased(ControllerButton button) const;
		
		// Axis queries
		float getAxisValue(ControllerAxis axis) const;
		glm::vec2 getLeftStick() const;
		glm::vec2 getRightStick() const;
		float getLeftTrigger() const;
		float getRightTrigger() const;
		
		// Deadzone configuration
		void setStickDeadzone(float deadzone) { _state.stickDeadzone = deadzone; }
		void setTriggerDeadzone(float deadzone) { _state.triggerDeadzone = deadzone; }
		float getStickDeadzone() const { return _state.stickDeadzone; }
		float getTriggerDeadzone() const { return _state.triggerDeadzone; }
		
		// Vibration/Rumble
		void setVibration(float leftMotor, float rightMotor, uint32_t durationMs = 0);
		void stopVibration();
		
		// Internal use
		void handleConnect(SDL_Gamepad* gamepad);
		void handleDisconnect();
		
	private:
		float applyDeadzone(float value, float deadzone) const;
		glm::vec2 applyRadialDeadzone(const glm::vec2& stick) const;
		
		int _playerIndex;
		SDL_Gamepad* _gamepad;
		SDL_JoystickID _joystickId;
		std::string _name;
		ControllerState _state;
		ControllerState _previousState;
	};
	
} // namespace noz
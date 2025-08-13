/*
	NoZ Game Engine
	
	Copyright(c) 2025 NoZ Games, LLC
*/

#include "internal/Controller.h"

namespace noz
{

Controller::Controller(int playerIndex, SDL_Gamepad* gamepad)
	: _playerIndex(playerIndex)
	, _gamepad(gamepad)
	, _joystickId(0)
	, _name("Unknown Controller")
{
	if (_gamepad)
	{
		_joystickId = SDL_GetGamepadID(_gamepad);
		const char* name = SDL_GetGamepadName(_gamepad);
		if (name)
		{
			_name = name;
		}
		
		Log::info("Controller", "Controller connected: " + _name + " (Player " + std::to_string(_playerIndex + 1) + ")");
	}
}

Controller::~Controller()
{
	if (_gamepad)
	{
		SDL_CloseGamepad(_gamepad);
		_gamepad = nullptr;
	}
}

void Controller::update()
{
	if (!_gamepad)
		return;
	
	// Save previous state
	_previousState = _state;
	
	// Update button states
	for (int i = 0; i < static_cast<int>(ControllerButton::Max); i++)
	{
		bool isPressed = SDL_GetGamepadButton(_gamepad, static_cast<SDL_GamepadButton>(i)) != 0;
		_state.buttons[i] = isPressed;
		_state.buttonsPressed[i] = isPressed && !_previousState.buttons[i];
		_state.buttonsReleased[i] = !isPressed && _previousState.buttons[i];
	}
	
	// Update axis values with deadzone
	for (int i = 0; i < static_cast<int>(ControllerAxis::Max); i++)
	{
		Sint16 rawValue = SDL_GetGamepadAxis(_gamepad, static_cast<SDL_GamepadAxis>(i));
		float normalizedValue = rawValue / 32767.0f;
		
		// Apply appropriate deadzone based on axis type
		if (i == static_cast<int>(ControllerAxis::TriggerLeft) || 
		    i == static_cast<int>(ControllerAxis::TriggerRight))
		{
			// Triggers go from 0 to 1
			normalizedValue = (normalizedValue + 1.0f) * 0.5f;
			normalizedValue = applyDeadzone(normalizedValue, _state.triggerDeadzone);
		}
		else
		{
			// Sticks go from -1 to 1
			normalizedValue = applyDeadzone(normalizedValue, _state.stickDeadzone);
		}
		
		_state.axes[i] = normalizedValue;
	}
}

bool Controller::isButtonPressed(ControllerButton button) const
{
	int index = static_cast<int>(button);
	if (index < 0 || index >= static_cast<int>(ControllerButton::Max))
		return false;
	return _state.buttons[index];
}

bool Controller::isButtonJustPressed(ControllerButton button) const
{
	int index = static_cast<int>(button);
	if (index < 0 || index >= static_cast<int>(ControllerButton::Max))
		return false;
	return _state.buttonsPressed[index];
}

bool Controller::isButtonJustReleased(ControllerButton button) const
{
	int index = static_cast<int>(button);
	if (index < 0 || index >= static_cast<int>(ControllerButton::Max))
		return false;
	return _state.buttonsReleased[index];
}

float Controller::getAxisValue(ControllerAxis axis) const
{
	int index = static_cast<int>(axis);
	if (index < 0 || index >= static_cast<int>(ControllerAxis::Max))
		return 0.0f;
	return _state.axes[index];
}

glm::vec2 Controller::getLeftStick() const
{
	glm::vec2 stick(
		_state.axes[static_cast<int>(ControllerAxis::LeftX)],
		_state.axes[static_cast<int>(ControllerAxis::LeftY)]
	);
	return applyRadialDeadzone(stick);
}

glm::vec2 Controller::getRightStick() const
{
	glm::vec2 stick(
		_state.axes[static_cast<int>(ControllerAxis::RightX)],
		_state.axes[static_cast<int>(ControllerAxis::RightY)]
	);
	return applyRadialDeadzone(stick);
}

float Controller::getLeftTrigger() const
{
	return _state.axes[static_cast<int>(ControllerAxis::TriggerLeft)];
}

float Controller::getRightTrigger() const
{
	return _state.axes[static_cast<int>(ControllerAxis::TriggerRight)];
}

void Controller::setVibration(float leftMotor, float rightMotor, uint32_t durationMs)
{
	if (!_gamepad)
		return;
	
	// Clamp values to 0-1 range
	leftMotor = std::clamp(leftMotor, 0.0f, 1.0f);
	rightMotor = std::clamp(rightMotor, 0.0f, 1.0f);
	
	// Convert to SDL's 0-65535 range
	Uint16 leftValue = static_cast<Uint16>(leftMotor * 65535);
	Uint16 rightValue = static_cast<Uint16>(rightMotor * 65535);
	
	SDL_RumbleGamepad(_gamepad, leftValue, rightValue, durationMs);
}

void Controller::stopVibration()
{
	if (_gamepad)
	{
		SDL_RumbleGamepad(_gamepad, 0, 0, 0);
	}
}

void Controller::handleConnect(SDL_Gamepad* gamepad)
{
	_gamepad = gamepad;
	if (_gamepad)
	{
		_joystickId = SDL_GetGamepadID(_gamepad);
		const char* name = SDL_GetGamepadName(_gamepad);
		if (name)
		{
			_name = name;
		}
		
		Log::info("Controller", "Controller reconnected: " + _name + " (Player " + std::to_string(_playerIndex + 1) + ")");
	}
}

void Controller::handleDisconnect()
{
	if (_gamepad)
	{
		Log::info("Controller", "Controller disconnected: " + _name + " (Player " + std::to_string(_playerIndex + 1) + ")");
		_gamepad = nullptr;
		_joystickId = 0;
		
		// Reset state
		_state = ControllerState();
		_previousState = ControllerState();
	}
}

float Controller::applyDeadzone(float value, float deadzone) const
{
	if (std::abs(value) < deadzone)
		return 0.0f;
	
	// Rescale the value to go from 0 to 1 after the deadzone
	float sign = value < 0 ? -1.0f : 1.0f;
	return sign * (std::abs(value) - deadzone) / (1.0f - deadzone);
}

glm::vec2 Controller::applyRadialDeadzone(const glm::vec2& stick) const
{
	float magnitude = glm::length(stick);
	if (magnitude < _state.stickDeadzone)
		return glm::vec2(0.0f);
	
	// Rescale the magnitude
	float scaledMagnitude = (magnitude - _state.stickDeadzone) / (1.0f - _state.stickDeadzone);
	scaledMagnitude = std::min(scaledMagnitude, 1.0f);
	
	// Apply the scaled magnitude to the normalized direction
	return glm::normalize(stick) * scaledMagnitude;
}

} // namespace noz
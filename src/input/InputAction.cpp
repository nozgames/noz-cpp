/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "noz/input/InputAction.h"

namespace noz
{

InputAction::InputAction(const std::string& name, InputActionType type)
	: _name(name)
	, _type(type)
	, _enabled(true)
	, _phase(InputActionPhase::Waiting)
	, _previousPhase(InputActionPhase::Waiting)
	, _currentValue(0.0f)
	, _previousValue(0.0f)
	, _currentValue2D(0.0f, 0.0f)
	, _previousValue2D(0.0f, 0.0f)
	, _buttonPressThreshold(0.5f)
{
}

InputAction::~InputAction()
{
}

void InputAction::update(float value, float deltaTime)
{
	if (!_enabled)
		return;
	
	_previousValue = _currentValue;
	_currentValue = value;
	
	bool isPressed = (value >= _buttonPressThreshold);
	updatePhase(isPressed, deltaTime);
}

void InputAction::update2D(const glm::vec2& value, float deltaTime)
{
	if (!_enabled)
		return;
	
	_previousValue2D = _currentValue2D;
	_currentValue2D = value;
	
	// For 2D inputs, consider it "pressed" if magnitude exceeds threshold
	bool isPressed = (glm::length(value) >= _buttonPressThreshold);
	updatePhase(isPressed, deltaTime);
}

void InputAction::reset()
{
	_phase = InputActionPhase::Waiting;
	_previousPhase = InputActionPhase::Waiting;
	_currentValue = 0.0f;
	_previousValue = 0.0f;
	_currentValue2D = glm::vec2(0.0f);
	_previousValue2D = glm::vec2(0.0f);
}

void InputAction::updatePhase(bool isPressed, float deltaTime)
{
	_previousPhase = _phase;
	
	switch (_phase)
	{
		case InputActionPhase::Waiting:
			if (isPressed)
			{
				_phase = InputActionPhase::Started;
				triggerCallback(InputActionPhase::Started, deltaTime);
			}
			break;
			
		case InputActionPhase::Started:
			if (isPressed)
			{
				_phase = InputActionPhase::Performed;
				triggerCallback(InputActionPhase::Performed, deltaTime);
			}
			else
			{
				_phase = InputActionPhase::Canceled;
				triggerCallback(InputActionPhase::Canceled, deltaTime);
			}
			break;
			
		case InputActionPhase::Performed:
			if (!isPressed)
			{
				_phase = InputActionPhase::Canceled;
				triggerCallback(InputActionPhase::Canceled, deltaTime);
			}
			else
			{
				// Continue performing
				triggerCallback(InputActionPhase::Performed, deltaTime);
			}
			break;
			
		case InputActionPhase::Canceled:
			if (isPressed)
			{
				_phase = InputActionPhase::Started;
				triggerCallback(InputActionPhase::Started, deltaTime);
			}
			else
			{
				_phase = InputActionPhase::Waiting;
			}
			break;
			
		case InputActionPhase::Disabled:
			// Do nothing
			break;
	}
}

void InputAction::triggerCallback(InputActionPhase phase, float deltaTime)
{
	if (!_callback)
		return;
	
	InputActionCallbackContext context;
	context.phase = phase;
	context.actionType = _type;
	context.value = _currentValue;
	context.value2D = _currentValue2D;
	context.deltaTime = deltaTime;
	context.performed = (phase == InputActionPhase::Performed);
	context.started = (phase == InputActionPhase::Started);
	context.canceled = (phase == InputActionPhase::Canceled);
	
	_callback(context);
}

} // namespace noz 
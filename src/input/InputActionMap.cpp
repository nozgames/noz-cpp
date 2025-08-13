/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include "noz/input/InputActionMap.h"
#include "noz/input/InputAction.h"

namespace noz
{

InputActionMap::InputActionMap(const std::string& name)
	: _name(name)
	, _enabled(true)
{
}

InputActionMap::~InputActionMap()
{
	clearBindings();
}

void InputActionMap::bindAction(InputCode inputCode, std::shared_ptr<InputAction> action)
{
	if (!action)
		return;

	_bindings[inputCode] = action;
	addActionToList(action);
}

void InputActionMap::unbindAction(InputCode inputCode)
{
	_bindings.erase(inputCode);
}

std::shared_ptr<InputAction> InputActionMap::createAction(const std::string& actionName, InputActionType type)
{
	auto action = std::make_shared<InputAction>(actionName, type);
	addActionToList(action);
	return action;
}

std::shared_ptr<InputAction> InputActionMap::createButtonAction(const std::string& actionName)
{
	return createAction(actionName, InputActionType::Button);
}

std::shared_ptr<InputAction> InputActionMap::createValueAction(const std::string& actionName)
{
	return createAction(actionName, InputActionType::Value);
}

void InputActionMap::clearBindings()
{
	_bindings.clear();
	_actions.clear();
}

std::shared_ptr<InputAction> InputActionMap::getAction(InputCode inputCode) const
{
	auto it = _bindings.find(inputCode);
	return (it != _bindings.end()) ? it->second : nullptr;
}

std::shared_ptr<InputAction> InputActionMap::getActionByName(const std::string& name) const
{
	for (const auto& action : _actions)
	{
		if (action->getName() == name)
			return action;
	}
	return nullptr;
}

void InputActionMap::enable()
{
	_enabled = true;
	for (auto& action : _actions)
	{
		action->enable();
	}
}

void InputActionMap::disable()
{
	_enabled = false;
	for (auto& action : _actions)
	{
		action->disable();
	}
}

void InputActionMap::addActionToList(std::shared_ptr<InputAction> action)
{
	// Check if already in list
	for (const auto& existing : _actions)
	{
		if (existing == action)
			return;
	}
	
	_actions.push_back(action);
}

} // namespace noz
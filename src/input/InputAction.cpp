/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz
{
    InputAction::InputAction(const std::string& name)
        : _name(name)
        , _isActive(false)
        , _wasActive(false)
    {
    }

    InputAction::~InputAction()
    {
    }

    void InputAction::onPressed()
    {
        updateState(true);
    }

    void InputAction::onReleased()
    {
        updateState(false);
    }

    void InputAction::updateState(bool newActive)
    {
        // Store previous state
        _wasActive = _isActive;
        
        // Update current state
        _isActive = newActive;

        // Handle state changes
        if (_isActive && !_wasActive)
        {
            // Action just started
            if (_beginCallback)
            {
                _beginCallback();
            }
        }
        else if (!_isActive && _wasActive)
        {
            // Action just ended
            if (_endCallback)
            {
                _endCallback();
            }
        }
    }

} // namespace noz 
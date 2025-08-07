/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

namespace noz::ui
{
    Button::Button()
        : Label()
        , _onClick(nullptr)
        , _isPressed(false)
        , _isHovered(false)
    {
        setName("Button");
    }

    void Button::update()
    {
        Label::update();
        
        // Handle input for this button
        handleInput();
    }

    void Button::renderElement(noz::renderer::CommandBuffer* commandBuffer)
    {
        // Create a copy of the style to modify for button states
        auto buttonStyle = style();
               
        // Temporarily set the modified style for rendering
        auto originalStyle = style();
        const_cast<Button*>(this)->setStyle(buttonStyle);
        
        // Render with modified style
        Label::renderElement(commandBuffer);
        
        // Restore original style
        const_cast<Button*>(this)->setStyle(originalStyle);
    }

    void Button::handleInput()
    {
        vec2 mousePos = getMousePosition();
        bool wasHovered = _isHovered;
        
        // Check if mouse is over this button
        _isHovered = isPointInBounds(mousePos);
        
        // Handle mouse clicks
        if (_isHovered && isMouseButtonJustPressed())
        {
            _isPressed = true;
            
            // Trigger click callback
            if (_onClick)
            {
                _onClick();
            }
        }
        else if (!isMouseButtonPressed())
        {
            _isPressed = false;
        }
        
        // Mark layout dirty if hover state changed (for visual feedback)
        if (wasHovered != _isHovered)
        {
            // Visual state changed, might need re-render
            // Note: In the original system, this would be handled by the UI phase system
        }
    }

    bool Button::isPointInBounds(const vec2& point) const
    {
        const auto& rect = bounds();
        return point.x >= rect.x && point.x <= rect.x + rect.width &&
               point.y >= rect.y && point.y <= rect.y + rect.height;
    }

    vec2 Button::getMousePosition() const
    {
        // TODO: Integrate with proper input system
        // For now, return a placeholder
        return vec2(0.0f, 0.0f);
    }

    bool Button::isMouseButtonPressed() const
    {
        // TODO: Integrate with proper input system
        // For now, return false
        return false;
    }

    bool Button::isMouseButtonJustPressed() const
    {
        // TODO: Integrate with proper input system
        // For now, return false
        return false;
    }
}
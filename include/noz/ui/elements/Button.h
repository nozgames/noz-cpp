/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::ui
{
    /**
     * @brief Button node that extends Label for interactive text buttons
     * Handles mouse input and click events
     */
    class Button : public Label
    {
    public:
        Button();
        virtual ~Button() = default;

        // Button-specific functionality
        void setOnClick(std::function<void()> onClick) { _onClick = onClick; }
        const std::function<void()>& onClick() const { return _onClick; }
        
        // Button state
        bool isPressed() const { return _isPressed; }
        bool isHovered() const { return _isHovered; }
        
        // Node lifecycle
        void update() override;

    protected:
        // Override rendering to show button states
        void renderElement(noz::renderer::CommandBuffer* commandBuffer) override;

    private:
        std::function<void()> _onClick;
        bool _isPressed = false;
        bool _isHovered = false;
        
        // Input handling
        void handleInput();
        bool isPointInBounds(const vec2& point) const;
        
        // TODO: Add proper input system integration
        // For now, we'll use placeholder methods
        vec2 getMousePosition() const;
        bool isMouseButtonPressed() const;
        bool isMouseButtonJustPressed() const;
    };
}
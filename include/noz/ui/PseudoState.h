/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <string>

namespace noz::ui
{
    enum class PseudoState : uint8_t
    {
        None = 0,
        Hover,
        Active,
        Selected,
        Disabled,
        Focused,
        Pressed,
        Checked
    };
    
    // Convert PseudoState to string for style lookup
    std::string pseudoStateToString(PseudoState state);
    
    // Parse PseudoState from string (for style sheet parsing)
    PseudoState stringToPseudoState(const std::string& str);
    
    // Helper to extract pseudo state from style name (e.g., "button:hover" -> "button", PseudoState::Hover)
    std::pair<std::string, PseudoState> parseStyleName(const std::string& styleName);
}
/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/ui/PseudoState.h>

namespace noz::ui
{
    std::string pseudoStateToString(PseudoState state)
    {
        switch (state)
        {
            case PseudoState::None:     return "";
            case PseudoState::Hover:    return "hover";
            case PseudoState::Active:   return "active";
            case PseudoState::Selected: return "selected";
            case PseudoState::Disabled: return "disabled";
            case PseudoState::Focused:  return "focused";
            case PseudoState::Pressed:  return "pressed";
            case PseudoState::Checked:  return "checked";
            default:                    return "";
        }
    }
    
    PseudoState stringToPseudoState(const std::string& str)
    {
        if (str == "hover")    return PseudoState::Hover;
        if (str == "active")   return PseudoState::Active;
        if (str == "selected") return PseudoState::Selected;
        if (str == "disabled") return PseudoState::Disabled;
        if (str == "focused")  return PseudoState::Focused;
        if (str == "pressed")  return PseudoState::Pressed;
        if (str == "checked")  return PseudoState::Checked;
        return PseudoState::None;
    }
    
    std::pair<std::string, PseudoState> parseStyleName(const std::string& styleName)
    {
        size_t colonPos = styleName.find(':');
        if (colonPos == std::string::npos)
        {
            return {styleName, PseudoState::None};
        }
        
        std::string baseName = styleName.substr(0, colonPos);
        std::string pseudoName = styleName.substr(colonPos + 1);
        PseudoState pseudoState = stringToPseudoState(pseudoName);
        
        return {baseName, pseudoState};
    }
}
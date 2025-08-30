//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "view_interface.h"
#include "../tui/tstring.h"
#include <vector>
#include <memory>

class PropertiesView : public IView
{
    struct Item
    {
        TString formatted_content;
        std::string raw_content;
        std::string name;
        std::string value;
        int indent_level;

        Item(const std::string& prop_name, const std::string& prop_value = "", int indent = 0)
            : raw_content(prop_name + (prop_value.empty() ? "" : ": " + prop_value))
            , name(prop_name)
            , value(prop_value)
            , indent_level(indent)
        {}
        
        Item(const std::string& prop_name, const TString& prop_value, int indent = 0)
            : raw_content(prop_name + ": " + prop_value.text)
            , name(prop_name)
            , value(prop_value.text)  // Convert TString to string for internal storage
            , indent_level(indent)
        {}
    };

    std::vector<std::unique_ptr<Item>> _properties;
    std::vector<Item*> _visible_properties;
    int _cursor_row = 0;
    bool _show_cursor = false;
    
    void RebuildVisibleList();
    void FormatProperty(Item* property);
    
public:
    void AddProperty(const std::string& name, const std::string& value = "", int indent_level = 0);
    void AddProperty(const std::string& name, const TString& value, int indent_level = 0);
    void Clear();
    size_t PropertyCount() const;
    
    // IView interface
    void Render(int width, int height) override;
    bool HandleKey(int key) override;
    void SetCursorVisible(bool visible) override;
    bool CanPopFromStack() const override { return false; }
    
    // Navigation support
    void ScrollUp(int lines = 1);
    void ScrollDown(int lines = 1);
    void ScrollToTop();
    void ScrollToBottom();
    
    void SetCursorPosition(int row, int col);
};
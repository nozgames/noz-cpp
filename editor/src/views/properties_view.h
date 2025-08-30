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
        std::string name;
        TString value;
    };

    std::vector<std::unique_ptr<Item>> _properties;
    std::vector<Item*> _visible_properties;
    int _cursor_row = 0;
    bool _show_cursor = false;
    
    void RebuildVisibleList();

public:

    void AddProperty(const std::string& name, const TString& value);
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
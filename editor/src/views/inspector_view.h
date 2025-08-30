//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "view_interface.h"
#include "tree_view.h"
#include "properties_view.h"
#include "inspector_object.h"
#include <memory>

class InspectorView : public IView
{
    std::unique_ptr<TreeView> _tree_view;
    std::unique_ptr<PropertiesView> _properties_view;
    bool _focus_on_tree = true;  // true = tree has focus, false = properties has focus
    int _split_position = 50;     // Percentage of width for tree view (0-100)
    std::unique_ptr<InspectorObject> _root_object;
    bool _has_requested_data = false;  // Track if we've sent a request yet
    
    void UpdateLayout(const irect_t& rect);
    void RenderDivider(const irect_t& rect, int split_col);
    void RenderPropertiesSection(int start_col, int properties_width, int height);
    void RefreshPropertiesFromSelectedNode();
    void BuildTreeFromInspectorObject(InspectorObject* obj);
    
public:
    InspectorView();
    ~InspectorView() = default;
    
    // InspectorObject management - the main API
    void SetRootObject(std::unique_ptr<InspectorObject> root);
    InspectorObject* GetRootObject() const { return _root_object.get(); }
    InspectorObject* GetSelectedObject() const;
    
    void ClearTree();
    void ResetRequestState(); // Reset the data request state
    void AddProperty(const std::string& name, const TString& value);
    void ClearProperties();

    // Layout control
    void SetSplitPosition(int percentage); // 0-100, percentage of width for tree
    void ToggleFocus();
    
    // IView interface
    void Render(const irect_t& rect) override;
    bool HandleKey(int key) override;
    void SetCursorVisible(bool visible) override;
    bool CanPopFromStack() const override { return true; }
    
    // Search support - delegates to tree view
    void SetSearchPattern(const std::string& pattern) override;
    void ClearSearch() override;
    bool SupportsSearch() const override;
    
    // Access to individual views (for advanced usage)
    TreeView* GetTreeView() const { return _tree_view.get(); }
    PropertiesView* GetPropertiesView() const { return _properties_view.get(); }
};
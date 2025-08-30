//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "view_interface.h"
#include "tree_view.h"
#include "properties_view.h"
#include "object_properties.h"
#include "inspector_object.h"
#include <memory>

class InspectorView : public IView
{
private:
    std::unique_ptr<TreeView> _tree_view;
    std::unique_ptr<PropertiesView> _properties_view;
    bool _focus_on_tree = true;  // true = tree has focus, false = properties has focus
    int _split_position = 50;     // Percentage of width for tree view (0-100)
    InspectorObject* _root_object = nullptr;
    
    void UpdateLayout(int width, int height);
    void RenderDivider(int width, int height, int split_col);
    void RenderPropertiesSection(int start_col, int properties_width, int height);
    void RefreshPropertiesFromSelectedNode();
    void BuildTreeFromInspectorObject(InspectorObject* obj);
    
public:
    InspectorView();
    ~InspectorView() = default;
    
    // InspectorObject management - the main API
    void SetRootObject(InspectorObject* root);
    InspectorObject* GetRootObject() const { return _root_object; }
    InspectorObject* GetSelectedObject() const;
    
    // Legacy tree operations (still available for manual tree building)
    void AddLine(const std::string& line);
    void AddObject(const std::string& name);
    void ClearTree();
    
    // Legacy properties operations
    void AddProperty(const std::string& name, const std::string& value = "", int indent_level = 0);
    void ClearProperties();
    
    // ObjectProperties management
    void SetCurrentNodeProperties(ObjectProperties* properties);
    void SetNodeProperties(const std::string& node_path, ObjectProperties* properties);
    ObjectProperties* GetCurrentNodeProperties() const;
    
    // Layout control
    void SetSplitPosition(int percentage); // 0-100, percentage of width for tree
    void ToggleFocus();
    
    // IView interface
    void Render(int width, int height) override;
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
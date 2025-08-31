//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "inspector_view.h"
#include "../tui/screen.h"
#include "../tui/terminal.h"
#include "../tokenizer.h"

extern void SendInspectRequest(const std::string& search_filter);
extern bool HasConnectedClient();

InspectorView::InspectorView()
    : _tree_view(std::make_unique<TreeView>())
    , _properties_view(std::make_unique<PropertiesView>())
{
    //_tree_view->Add(TStringBuilder().Add("Waiting for client...").ToString());
}

void InspectorView::SetRootObject(std::unique_ptr<InspectorObject> root)
{
    _root_object = std::move(root);

    if (_root_object)
        BuildTreeFromInspectorObject(_root_object.get());
    else
        ClearTree();
}

InspectorObject* InspectorView::GetSelectedObject() const
{
    TreeNode* current_node = _tree_view->GetCurrentNode();
    if (current_node)
    {
        return static_cast<InspectorObject*>(current_node->GetUserData());
    }
    return nullptr;
}

void InspectorView::ClearTree()
{
    _tree_view->Clear();
}

void InspectorView::ResetRequestState()
{
    _has_requested_data = false;
}

void InspectorView::AddProperty(const std::string& name, const TString& value)
{
    // Add property to current node's ObjectProperties
    TreeNode* current_node = _tree_view->GetCurrentNode();
    if (!current_node) return;
    
    ObjectProperties* props = static_cast<ObjectProperties*>(current_node->GetUserData());
    if (!props)
    {
        // Create new ObjectProperties for this node
        props = new ObjectProperties();
        current_node->SetUserData(props);
    }
    
    //props->AddProperty(name, value);
    RefreshPropertiesFromSelectedNode();
}

void InspectorView::ClearProperties()
{
    TreeNode* current_node = _tree_view->GetCurrentNode();
    if (current_node)
    {
        ObjectProperties* props = static_cast<ObjectProperties*>(current_node->GetUserData());
        if (props)
        {
            props->Clear();
            RefreshPropertiesFromSelectedNode();
        }
    }
}

void InspectorView::SetSplitPosition(int percentage)
{
    _split_position = std::max(10, std::min(90, percentage)); // Clamp to 10-90%
}

void InspectorView::ToggleFocus()
{
    _focus_on_tree = !_focus_on_tree;
    _tree_view->SetCursorVisible(_focus_on_tree);
    _properties_view->SetCursorVisible(!_focus_on_tree);
}

void InspectorView::UpdateLayout(const RectInt& rect)
{
    // Calculate split column
    int split_col = (rect.width * _split_position) / 100;
    split_col = std::max(5, std::min(rect.width - 5, split_col));
}

void InspectorView::RenderDivider(const RectInt& rect, int split_col)
{
    //SetColorRGB(255, 255, 255, 128, 128, 128);  // White text, gray background
    DrawVerticalLine(rect.x + split_col, rect.y, rect.height, '|', TCOLOR_WHITE);
}

void InspectorView::RenderPropertiesSection(int start_col, int properties_width, int height)
{
#if 0
    if (!_properties_view) return;
    
    int properties_height = height - 2; // Leave 2 rows for status and command
    int render_start_col = start_col + 1; // Add space after divider
    int available_width = properties_width - 1;
    
    // Clear the properties area
    for (int row = 0; row < properties_height; row++)
    {
        MoveCursor(row, start_col);
        for (int col = 0; col < properties_width; col++)
            AddChar(' ');
    }
    
    // Get current selection and render its properties
    InspectorObject* selected_obj = GetSelectedObject();
    if (!selected_obj)
    {
        MoveCursor(0, render_start_col);
        AddString("(No selection)");
        return;
    }
    
    const ObjectProperties& props = selected_obj->GetProperties();
    if (props.IsEmpty())
    {
        MoveCursor(0, render_start_col);
        AddString("(No properties)");
        MoveCursor(1, render_start_col);
        AddString(("Type: " + selected_obj->GetType()).c_str());
        AddString(("Name: " + selected_obj->GetType()).c_str());
        return;
    }
    
    // Use 50% of available width for property names
    size_t max_name_length = available_width / 2;
    
    // Show object name (aligned)
    MoveCursor(0, render_start_col);
    std::string object_label = "Object";
    AddString(object_label.c_str());
    
    // Pad to alignment
    for (size_t pad = object_label.length(); pad < max_name_length; pad++)
        AddChar(' ');
    AddString(" : ");
    AddString(selected_obj->GetName().c_str());
    
    // Render properties with alignment
    int current_row = 1;
    for (size_t i = 0; i < props.Count() && current_row < properties_height; i++)
    {
        const ObjectProperty& prop = props.GetProperty(i);
        
        MoveCursor(current_row, render_start_col);
        AddString(prop.name.c_str());
        AddString(" : ");
        
        // Calculate remaining width for value after alignment
        int remaining_width = available_width - static_cast<int>(max_name_length) - 2;
        if (remaining_width > 0)
            AddString(prop.value,remaining_width);

        current_row++;
    }
#endif
}

void InspectorView::RefreshPropertiesFromSelectedNode()
{
    TreeNode* current_node = _tree_view->GetCurrentNode();
    if (!current_node)
    {
        _properties_view->Clear();
        return;
    }
    
    void* user_data = current_node->GetUserData();
    if (!user_data)
    {
        _properties_view->Clear();
        return;
    }
    
    // Cast to InspectorObject with additional validation
    InspectorObject* selected_obj = static_cast<InspectorObject*>(user_data);
    
    // Try to access a member to see if the object is valid
    // This will crash if it's garbage, but at least we'll know where
    try 
    {
        const ObjectProperties& props = selected_obj->GetProperties();
        
        // Clear and repopulate properties view
        _properties_view->Clear();
        for (size_t i = 0; i < props.Count(); i++)
        {
            const ObjectProperty& prop = props.GetProperty(i);
            //_properties_view->AddProperty(prop.name, prop.value);
        }
    }
    catch (...)
    {
        // If we get here, the InspectorObject pointer was garbage
        _properties_view->Clear();
        // Could add logging here if available
    }
}

void InspectorView::BuildTreeFromInspectorObject(InspectorObject* obj)
{
#if 0
    assert(obj);

    _tree_view->Clear();
    
    std::function<void(InspectorObject*, int)> build_recursive = [&](InspectorObject* inspector_obj, int indent)
    {
        assert(inspector_obj);

        auto builder = TStringBuilder();
        if (inspector_obj->IsEnabled())
            builder.Add(inspector_obj->GetType());
        else
            builder.Add(inspector_obj->GetType(), TCOLOR_DISABLED);

        _tree_view->Add(builder.ToString(), indent, inspector_obj);
        
        size_t child_count = inspector_obj->GetChildCount();
        for (size_t i = 0; i < child_count; i++)
            if (InspectorObject* child = inspector_obj->GetChild(i))
                build_recursive(child, indent + 1);
    };
    
    build_recursive(obj, 0);
#endif
}

void InspectorView::Render(const RectInt& rect)
{
    PushClipRect(rect);

    // Check if we haven't requested data yet and if a client is now connected
    if (!_has_requested_data)
    {
        if (HasConnectedClient())
        {
            // Client connected - send the inspection request
            SendInspectRequest("");
            _has_requested_data = true;
        }
    }
    
    UpdateLayout(rect);
    
    // Check if tree cursor has changed and refresh properties if so
    if (_tree_view->HasCursorChanged())
    {
        RefreshPropertiesFromSelectedNode();
        _tree_view->MarkCursorProcessed();
    }

#if 0
    int split_col = (width * _split_position) / 100;
    split_col = std::max(width / 2, std::min(width - 5, split_col));  // Minimum 50% for tree, maximum leave 5 for properties
    
    int tree_width = split_col;
    int properties_width = width - split_col - 1; // -1 for divider
    
    // Clear the entire area first
    for (int row = 0; row < height - 2; row++)
    {
        MoveCursor(row, 0);
        for (int col = 0; col < width; col++)
            AddChar(' ');
    }
    
    // Render tree view (left side)
    _tree_view->Render(tree_width, height);
#endif

    int split_col = (rect.width * _split_position) / 100;
    split_col = std::max(rect.width / 2, std::min(rect.width - 5, split_col));  // Minimum 50% for tree, maximum leave 5 for properties

    _tree_view->Render({rect.x, rect.y, split_col - 1, rect.height});

    // Render divider
    RenderDivider(rect, split_col);

#if 0
    // Render properties view (right side) manually
    RenderPropertiesSection(split_col + 1, properties_width, height);
#endif

    PopClipRect();
}

bool InspectorView::HandleKey(int key)
{
    switch (key)
    {
        case '\t':  // Tab to switch focus
            ToggleFocus();
            return true;
            
        case KEY_F1:  // F1 to adjust split
            _split_position = std::max(10, _split_position - 5);
            return true;
            
        case KEY_F2:  // F2 to adjust split
            _split_position = std::min(90, _split_position + 5);
            return true;
            
        default:
            // Delegate to focused view
            if (_focus_on_tree)
                return _tree_view->HandleKey(key);
            else
                return _properties_view->HandleKey(key);
    }
}

void InspectorView::SetCursorVisible(bool visible)
{
    if (_focus_on_tree)
    {
        _tree_view->SetCursorVisible(visible);
        _properties_view->SetCursorVisible(false);
    }
    else
    {
        _tree_view->SetCursorVisible(false);
        _properties_view->SetCursorVisible(visible);
    }
}

void InspectorView::SetSearchPattern(const std::string& pattern)
{
    _tree_view->SetSearchPattern(pattern);
}

void InspectorView::ClearSearch()
{
    _tree_view->ClearSearch();
}

bool InspectorView::SupportsSearch() const
{
    return _tree_view->SupportsSearch();
}
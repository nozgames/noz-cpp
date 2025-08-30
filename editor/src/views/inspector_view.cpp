//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "inspector_view.h"
#include "../tui/terminal.h"
#include "../tokenizer.h"
#include <algorithm>

static void FormatValue(TStringBuilder& builder, const std::string& value)
{
    if (value.empty())
    {
        builder.Add(value);
        return;
    }
    
    Tokenizer tok;
    Token token;
    
    // Check for color patterns using tokenizer
    color_t color_result;
    Init(tok, value.c_str());
    if (ExpectColor(tok, &token, &color_result))
    {
        builder.Add(color_result);
        return;
    }
    
    // Check for vector patterns using tokenizer: (x,y), (x,y,z), or (x,y,z,w)
    if (value.size() >= 5 && value.front() == '(' && value.back() == ')')
    {
        vec2 vec2_result;
        vec3 vec3_result;
        vec4 vec4_result;
        
        // Try parsing as vec2 first
        Init(tok, value.c_str());
        if (ExpectVec2(tok, &token, &vec2_result))
        {
            builder.Add(vec2_result);
            return;
        }
        
        // Reset tokenizer and try vec3
        Init(tok, value.c_str());
        if (ExpectVec3(tok, &token, &vec3_result))
        {
            builder.Add(vec3_result);
            return;
        }
        
        // Reset tokenizer and try vec4
        Init(tok, value.c_str());
        if (ExpectVec4(tok, &token, &vec4_result))
        {
            builder.Add(vec4_result);
            return;
        }
    }
    
    // Check for boolean values
    if (value == "true" || value == "false")
    {
        builder.Add(value == "true");
        return;
    }
    
    // Check for number (integer or float)
    static const std::regex number_regex("^[-+]?([0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)$");
    if (std::regex_match(value, number_regex))
    {
        // Try parsing as int first, then float
        char* end;
        long int_val = strtol(value.c_str(), &end, 10);
        if (*end == '\0')
            builder.Add(static_cast<int>(int_val));
        else
            builder.Add(static_cast<float>(strtof(value.c_str(), nullptr)));
        return;
    }
    
    // Everything else is treated as a string
    if (!value.empty() && value.front() == '"' && value.back() == '"')
        builder.Add(value, TCOLOR_GREEN); // Already has quotes
    else
        builder.Add("\"" + value + "\"", TCOLOR_GREEN); // Add quotes
}

InspectorView::InspectorView()
    : _tree_view(std::make_unique<TreeView>())
    , _properties_view(std::make_unique<PropertiesView>())
{
    // Show waiting message initially
    _tree_view->AddLine("Waiting for client...");
    
    // Request inspection data from connected clients
    // This will be ignored if no client is connected, but will populate
    // the inspector if a client is available
    extern void SendInspectRequest(const std::string& search_filter);
    SendInspectRequest("");
}

void InspectorView::SetRootObject(InspectorObject* root)
{
    _root_object = root;
    if (root)
    {
        BuildTreeFromInspectorObject(root);
    }
    else
    {
        ClearTree();
    }
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

void InspectorView::AddLine(const std::string& line)
{
    _tree_view->AddLine(line);
}

void InspectorView::AddObject(const std::string& name)
{
    _tree_view->AddObject(name);
}


void InspectorView::ClearTree()
{
    _tree_view->Clear();
}

void InspectorView::AddProperty(const std::string& name, const std::string& value, int indent_level)
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
    
    props->AddProperty(name, value, indent_level);
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

void InspectorView::SetCurrentNodeProperties(ObjectProperties* properties)
{
    _tree_view->SetCurrentNodeUserData(properties);
    RefreshPropertiesFromSelectedNode();
}

void InspectorView::SetNodeProperties(const std::string& node_path, ObjectProperties* properties)
{
    _tree_view->SetNodeUserData(node_path, properties);
    RefreshPropertiesFromSelectedNode();
}

ObjectProperties* InspectorView::GetCurrentNodeProperties() const
{
    TreeNode* current_node = _tree_view->GetCurrentNode();
    if (current_node)
    {
        return static_cast<ObjectProperties*>(current_node->GetUserData());
    }
    return nullptr;
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

void InspectorView::UpdateLayout(int width, int height)
{
    // Calculate split column
    int split_col = (width * _split_position) / 100;
    split_col = std::max(5, std::min(width - 5, split_col)); // Ensure minimum space for both views
}

void InspectorView::RenderDivider(int width, int height, int split_col)
{
    // Render vertical divider between tree and properties using background color
    for (int row = 0; row < height - 2; row++) // Leave 2 rows for status and command
    {
        MoveCursor(row, split_col);
        SetColorRGB(255, 255, 255, 128, 128, 128);  // White text, gray background
        AddChar(' ');  // Space character with background
        EndColor();  // Reset color
    }
}

void InspectorView::RenderPropertiesSection(int start_col, int properties_width, int height)
{
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
        AddString(("Object: " + selected_obj->GetName()).c_str());
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
        
        // Add indentation and property name
        std::string indented_name;
        for (int indent = 0; indent < prop.indent_level; indent++)
            indented_name += "  ";
        indented_name += prop.name;
        
        AddString(indented_name.c_str());
        
        // Pad to alignment
        for (size_t pad = indented_name.length(); pad < max_name_length; pad++)
            AddChar(' ');
        
        AddString(" : ");
        
        // Calculate remaining width for value after alignment
        int value_start_pos = render_start_col + static_cast<int>(max_name_length) + 2; // +2 for ": "
        int remaining_width = available_width - static_cast<int>(max_name_length) - 2;
        
        if (remaining_width > 0)
        {
            // Format the value
            auto builder = TStringBuilder::Build();
            FormatValue(builder, prop.value.text);
            
            // Truncate to fit remaining width
            builder.TruncateToWidth(remaining_width);
            TString formatted_value = builder.ToString();
            
            // Render the formatted value
            AddString(formatted_value);
        }
        
        current_row++;
    }
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
            _properties_view->AddProperty(prop.name, prop.value, prop.indent_level);  // prop.value is now TString
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
    if (!obj) return;
    
    // Clear the tree first
    _tree_view->Clear();
    
    // Recursively build tree structure with explicit indent levels
    std::function<void(InspectorObject*, int)> build_recursive = [&](InspectorObject* inspector_obj, int indent) {
        if (!inspector_obj) return;
        
        // Add this object to the tree with user data attached directly
        _tree_view->Add(inspector_obj->GetName(), indent, inspector_obj);
        
        // Debug: Log child count for this object
        size_t child_count = inspector_obj->GetChildCount();
        // TODO: Add proper logging when available - for now just check child_count
        
        // Add all children at the next indent level
        for (size_t i = 0; i < child_count; i++)
            if (InspectorObject* child = inspector_obj->GetChild(i))
                build_recursive(child, indent + 1);
    };
    
    build_recursive(obj, 0);  // Start at indent level 0
}

void InspectorView::Render(int width, int height)
{
    UpdateLayout(width, height);
    
    // Check if tree cursor has changed and refresh properties if so
    if (_tree_view->HasCursorChanged())
    {
        RefreshPropertiesFromSelectedNode();
        _tree_view->MarkCursorProcessed();
    }
    
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
    
    // Render divider
    RenderDivider(width, height, split_col);
    
    // Render properties view (right side) manually
    RenderPropertiesSection(split_col + 1, properties_width, height);
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
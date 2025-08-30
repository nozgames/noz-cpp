//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "object_properties.h"

class InspectorObject
{
    std::string _name;
    ObjectProperties _properties;
    std::vector<std::unique_ptr<InspectorObject>> _children;
    InspectorObject* _parent = nullptr;

public:
    InspectorObject(const std::string& name) : _name(name) {}
    
    // Name management
    const std::string& GetName() const { return _name; }
    void SetName(const std::string& name) { _name = name; }
    
    // Property management
    void AddProperty(const std::string& name, const std::string& value);
    void AddProperty(const std::string& name, const TString& value);

    const ObjectProperty* FindProperty(const std::string& name) const;

    const ObjectProperties& GetProperties() const { return _properties; }
    ObjectProperties& GetProperties() { return _properties; }
    
    // Hierarchy management
    InspectorObject* AddChild(const std::string& name)
    {
        auto child = std::make_unique<InspectorObject>(name);
        child->_parent = this;
        InspectorObject* ptr = child.get();
        _children.push_back(std::move(child));
        return ptr;
    }
    
    InspectorObject* AddChild(std::unique_ptr<InspectorObject> child)
    {
        child->_parent = this;
        InspectorObject* ptr = child.get();
        _children.push_back(std::move(child));
        return ptr;
    }
    
    size_t GetChildCount() const { return _children.size(); }
    bool HasChildren() const { return !_children.empty(); }
    
    InspectorObject* GetChild(size_t index) const
    {
        if (index < _children.size())
            return _children[index].get();
        return nullptr;
    }
    
    const std::vector<std::unique_ptr<InspectorObject>>& GetChildren() const
    {
        return _children;
    }
    
    InspectorObject* GetParent() const { return _parent; }
    
    // Helper to find child by name
    InspectorObject* FindChild(const std::string& name) const
    {
        for (const auto& child : _children)
        {
            if (child->GetName() == name)
                return child.get();
        }
        return nullptr;
    }
    
    // Recursive find by path (e.g., "Player System/Movement Component")
    InspectorObject* FindByPath(const std::string& path) const
    {
        size_t slash_pos = path.find('/');
        if (slash_pos == std::string::npos)
        {
            // No slash, look for direct child
            return FindChild(path);
        }
        
        // Split path and recurse
        std::string first_part = path.substr(0, slash_pos);
        std::string remaining_path = path.substr(slash_pos + 1);
        
        InspectorObject* child = FindChild(first_part);
        if (child)
            return child->FindByPath(remaining_path);
            
        return nullptr;
    }
    
    // Get full path from root
    std::string GetPath() const
    {
        if (!_parent)
            return _name;
            
        std::string parent_path = _parent->GetPath();
        if (parent_path.empty())
            return _name;
        return parent_path + "/" + _name;
    }
    
    // Clear all children
    void ClearChildren()
    {
        _children.clear();
    }

    static std::unique_ptr<InspectorObject> CreateFromStream(Stream* stream);
};
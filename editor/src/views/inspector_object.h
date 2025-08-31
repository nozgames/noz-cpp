//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "object_properties.h"

class InspectorObject
{
    std::string _name;
    std::string _type;
    ObjectProperties _properties;
    std::vector<std::unique_ptr<InspectorObject>> _children;
    InspectorObject* _parent = nullptr;
    bool _enabled;

public:

    InspectorObject(const std::string& type, const std::string& name) : _name(name), _type(type) {}
    
    const std::string& GetName() const { return _name; }
    const std::string& GetType() const { return _type; }
    void AddProperty(const std::string& name, const std::string& value);
    void AddProperty(const std::string& name, TString* value);
    const ObjectProperty* FindProperty(const std::string& name) const;
    const ObjectProperties& GetProperties() const { return _properties; }
    ObjectProperties& GetProperties() { return _properties; }
    InspectorObject* AddChild(std::unique_ptr<InspectorObject> child);
    size_t GetChildCount() const { return _children.size(); }
    bool HasChildren() const { return !_children.empty(); }
    InspectorObject* GetChild(size_t index) const;
    bool IsEnabled() const { return _enabled; }
    
    const std::vector<std::unique_ptr<InspectorObject>>& GetChildren() const
    {
        return _children;
    }
    
    static std::unique_ptr<InspectorObject> CreateFromStream(Stream* stream);
};

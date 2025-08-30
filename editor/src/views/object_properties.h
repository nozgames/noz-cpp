//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include <vector>
#include <string>
#include "../tui/tstring.h"

struct ObjectProperty
{
    std::string name;
    TString value;
    int indent_level = 0;
    
    ObjectProperty(const std::string& prop_name, const std::string& prop_value = "", int indent = 0)
        : name(prop_name), value(prop_value, prop_value.length()), indent_level(indent) {}
        
    ObjectProperty(const std::string& prop_name, const TString& prop_value, int indent = 0)
        : name(prop_name), value(prop_value), indent_level(indent) {}
};

class ObjectProperties
{
private:
    std::vector<ObjectProperty> _properties;
    
public:
    void AddProperty(const std::string& name, const std::string& value = "", int indent_level = 0)
    {
        _properties.emplace_back(name, value, indent_level);
    }
    
    void AddProperty(const std::string& name, const TString& value, int indent_level = 0)
    {
        _properties.emplace_back(name, value, indent_level);
    }
    
    void Clear()
    {
        _properties.clear();
    }
    
    size_t Count() const
    {
        return _properties.size();
    }
    
    const ObjectProperty& GetProperty(size_t index) const
    {
        return _properties[index];
    }
    
    const std::vector<ObjectProperty>& GetProperties() const
    {
        return _properties;
    }
    
    bool IsEmpty() const
    {
        return _properties.empty();
    }
    
    // Helper method to find property by name
    const ObjectProperty* FindProperty(const std::string& name) const
    {
        for (const auto& prop : _properties)
        {
            if (prop.name == name)
                return &prop;
        }
        return nullptr;
    }
    
    // Helper method to set/update a property
    void SetProperty(const std::string& name, const std::string& value, int indent_level = 0)
    {
        for (auto& prop : _properties)
        {
            if (prop.name == name)
            {
                prop.value = TString(value, value.length());
                prop.indent_level = indent_level;
                return;
            }
        }
        // Property not found, add it
        AddProperty(name, value, indent_level);
    }
    
    void SetProperty(const std::string& name, const TString& value, int indent_level = 0)
    {
        for (auto& prop : _properties)
        {
            if (prop.name == name)
            {
                prop.value = value;
                prop.indent_level = indent_level;
                return;
            }
        }
        // Property not found, add it
        AddProperty(name, value, indent_level);
    }
};
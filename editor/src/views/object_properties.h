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
};

class ObjectProperties
{
    std::vector<ObjectProperty> _properties;
    
public:

    void AddProperty(const std::string& name, const TString& value)
    {
        _properties.emplace_back(name, value);
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
    
    const ObjectProperty* FindProperty(const std::string& name) const;
    void SetProperty(const std::string& name, const std::string& value);
    void SetProperty(const std::string& name, const TString& value);
};
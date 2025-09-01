#pragma once

struct Name
{
    const char* value;
};

Name* GetName(const char* value);
inline const char* GetString(Name* name) { return name->value; }
inline const char* GetValue(const Name* name, const char* default_value = "")
{
    return name && name->value ? name->value : default_value;
}

#pragma once

struct name_t
{
    const char* value;
};

name_t* GetName(const char* value);
inline const char* GetString(name_t* name) { return name->value; }
inline const char* GetValue(const name_t* name, const char* default_value = "")
{
    return name && name->value ? name->value : default_value;
}

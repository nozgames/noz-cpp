#pragma once

constexpr u32 MAX_NAME_LENGTH = 64;

struct Name
{
    char value[MAX_NAME_LENGTH];
};

Name* GetName(const char* value);
inline const char* GetString(const Name* name) {
    return name->value;
}

inline const char* GetValue(const Name* name, const char* default_value = "") {
    return name && name->value ? name->value : default_value;
}

extern Name* NAME_NONE;

//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

constexpr u32 MAX_NAME_LENGTH = 128;

struct Name {
    char value[MAX_NAME_LENGTH];
};

extern Name* GetName(const char* value);

inline const char* GetString(const Name* name) {
    return name->value;
}

inline const char* GetValue(const Name* name, const char* default_value = "") {
    return name ? name->value : default_value;
}

extern Name* NAME_NONE;

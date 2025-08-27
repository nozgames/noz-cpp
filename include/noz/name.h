#pragma once

struct name_t
{
    const char* value;
};

name_t* GetName(const char* value);
inline const char* GetString(name_t* name) { return name->value; }

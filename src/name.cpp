#include <noz/name.h>

static Allocator* g_name_allocator = nullptr;
static Map g_names_map = {};
static u64* g_names_map_keys = nullptr;
static Name* g_names_map_values = nullptr;
static Name g_name_none = {""};

Name* NAME_NONE;

Name* GetName(const char* value)
{
    if (!value || *value == 0)
        return NAME_NONE;

    auto key = Hash(value);
    auto name = (Name*)GetValue(g_names_map, key);
    if (name)
        return name;

    auto name_value_len = strlen(value);
    auto name_value = (char*)Alloc(g_name_allocator, name_value_len + 1);
    memcpy(name_value, value, name_value_len + 1);
    CleanPath(name_value);
    name = (Name*)SetValue(g_names_map, key);
    name->value = name_value;
    return name;;
}

void InitName(ApplicationTraits* traits)
{
    g_name_allocator = CreateArenaAllocator(traits->name_memory_size, "name");

    g_names_map_keys = (u64*)Alloc(g_name_allocator, sizeof(u64) * traits->max_names);
    g_names_map_values = (Name*)Alloc(g_name_allocator, sizeof(Name) * traits->max_names);
    Init(g_names_map, g_names_map_keys, g_names_map_values, traits->max_names, sizeof(Name), 0);

    NAME_NONE = &g_name_none;
}

void ShutdownName()
{
    g_names_map_keys = nullptr;
    g_names_map_values = nullptr;
    Clear(g_name_allocator);
    Destroy(g_name_allocator);
    g_name_allocator = nullptr;
    g_names_map = {};
}
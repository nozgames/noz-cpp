//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

const TypeTraits* g_type_traits[TYPE_COUNT] = {};
const char* g_type_names[TYPE_COUNT] = {};
static TypeTraits g_default_type_traits = {};
static const TypeTraits* g_default_type_traits_ptr = &g_default_type_traits;

void SetTypeTraits(type_t id, const TypeTraits* traits)
{
    assert(g_type_traits[id] == g_default_type_traits_ptr);
    g_type_traits[id] = traits;
}

void SetTypeName(type_t type, const char* name)
{
    g_type_names[type] = name;
}

void InitTypes()
{
    const char* unknown = "Unknown";
    for (int i=0; i<TYPE_COUNT; i++)
    {
        g_type_traits[i] = g_default_type_traits_ptr;
        g_type_names[i] = unknown;
    }

#define NOZ_TYPE(type) g_type_names[TYPE_##type] = #type;
    NOZ_CORE_TYPES
#undef NOZ_TYPE
}

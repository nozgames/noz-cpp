//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

const TypeTraits* g_type_traits[TYPE_COUNT] = {};
static TypeTraits g_default_type_traits = {};
static const TypeTraits* g_default_type_traits_ptr = &g_default_type_traits;

void SetTypeTraits(type_t id, const TypeTraits* traits)
{
    assert(g_type_traits[id] == g_default_type_traits_ptr);
    g_type_traits[id] = traits;
}

void InitTypes()
{
    for (int i=0; i<TYPE_COUNT; i++)
        g_type_traits[i] = g_default_type_traits_ptr;
}
//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct ComponentImpl : Object
{
    Entity* entity;
    LinkedListNode node_entity;
};

static_assert(COMPONENT_BASE_SIZE == sizeof(ComponentImpl));

const ComponentTraits* g_component_traits[TYPE_COUNT] = {};

static ComponentImpl* Impl(Component* component) { return (ComponentImpl*)CastToBase(component, TYPE_COMPONENT); }

void SetComponentTraits(type_t id, const ComponentTraits* traits)
{
    g_component_traits[id] = traits;
}

Component* CreateComponent(Allocator* allocator, size_t component_size, type_t component_type)
{
    auto component = (Component*)CreateObject(allocator, component_size, component_type, TYPE_COMPONENT);
    auto impl = Impl(component);
    return component;
}

Entity* GetEntity(Component* component)
{
    return Impl(component)->entity;
}

void InitComponentList(LinkedList& list)
{
    Init(list, offsetof(ComponentImpl, node_entity));
}

void SetEntity(Component* component, Entity* entity)
{
    auto impl = Impl(component);
    impl->entity = entity;
}

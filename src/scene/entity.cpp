//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct EntityImpl
{
    OBJECT_BASE;
    Entity* parent;
    vec3 local_position;
    vec3 local_scale;
    quat local_rotation;
    mat4 local_to_world;
    mat4 world_to_local;
    LinkedList children;
    LinkedListNode node_child;
    LinkedListNode node_render;

    // todo: bools?
    bool local_to_world_dirty;
    bool world_to_local_dirty;
    bool enabled;
    bool enabled_in_hierarchy;
    u32 version;
    // const entity_traits* traits = nullptr;
};

static_assert(ENTITY_BASE_SIZE == sizeof(EntityImpl));

const EntityTraits* g_entity_traits[TYPE_COUNT] = {};
static LinkedList g_render_entities = {};

void SetEntityTraits(type_t id, const EntityTraits* traits)
{
    g_entity_traits[id] = traits;
}

static EntityImpl* Impl(Entity* e) { return (EntityImpl*)CastToBase(e, TYPE_ENTITY); }

static void UpdateRenderEntity(Entity* entity)
{
    auto traits = GetEntityTraits(entity);
    if (traits->render == nullptr)
        return;
    auto should_render = IsEnabled(entity);
    auto is_rendering = IsInList(g_render_entities, entity);
    if (should_render && !is_rendering)
        PushBack(g_render_entities, entity);
    else if (!should_render && is_rendering)
        Remove(g_render_entities, entity);
}

static void MarkDirty(EntityImpl* impl, bool force)
{
    assert(impl);

    if (!force && impl->local_to_world_dirty)
        return;

    impl->local_to_world_dirty = true;
    impl->world_to_local_dirty = true;
    impl->version++;

    for (auto child=GetFront(impl->children); child; child=GetNext(impl->children, child))
        MarkDirty((EntityImpl*)child, force);
}

static void UpdateLocalToWorld(EntityImpl* impl)
{
    assert(impl);

    auto local_to_world =
        translate(impl->local_position) *
        mat4_cast(impl->local_rotation) *
        scale(impl->local_scale);

    if (impl->parent)
        impl->local_to_world = GetLocalToWorld(impl->parent) * local_to_world;
    else
        impl->local_to_world = local_to_world;

    impl->local_to_world_dirty = false;
    impl->world_to_local_dirty = true;
}

static void UpdateWorldToLocal(EntityImpl* impl)
{
    if (impl->local_to_world_dirty)
        UpdateLocalToWorld(impl);

    impl->world_to_local = inverse(impl->local_to_world);
    impl->world_to_local_dirty = false;
}

vec3 GetLocalPosition(Entity* entity)
{
    return Impl(entity)->local_position;
}

vec3 GetWorldPosition(Entity* entity)
{
    return GetLocalToWorld(entity)[3];
}

void SetLocalPosition(Entity* entity, const vec3& pos)
{
    auto impl = Impl(entity);
    impl->local_position = pos;
    MarkDirty(impl, false);
}

void SetLocalScale(Entity* entity, const vec3& scale)
{
    auto impl = Impl(entity);
    impl->local_scale = scale;
    MarkDirty(impl, false);
}

void SetLocalScale(Entity* entity, float scale)
{
    auto impl = Impl(entity);
    impl->local_scale = vec3(scale, scale, scale);
    MarkDirty(impl, false);
}

void SetWorldPosition(Entity* e, float x, float y, float z)
{
    SetWorldPosition(e, vec3(x,y,z));
}

void SetWorldPosition(Entity* e, const vec3& world_position)
{
    auto impl = Impl(e);
    if (impl->parent)
        impl->local_position = InverseTransformPoint(impl->parent, world_position);
    else
        impl->local_position = world_position;

    MarkDirty(impl, false);
}

const mat4& GetWorldToLocal(Entity* entity)
{
    auto impl = Impl(entity);
    if (impl->world_to_local_dirty)
        UpdateWorldToLocal(impl);

    return impl->world_to_local;
}

const mat4& GetLocalToWorld(Entity* entity)
{
    EntityImpl* impl = Impl(entity);
    if (impl->local_to_world_dirty)
        UpdateLocalToWorld(impl);

    return impl->local_to_world;
}

void SetLocalRotation(Entity* entity, const quat& rotation)
{
    auto impl = Impl(entity);
    impl->local_rotation = rotation;
    MarkDirty(impl, false);
}

void SetLocalEulerAngles(Entity* entity, const vec3& angles_in_degrees)
{
    SetLocalRotation(entity, quat(radians(angles_in_degrees)));
}

void SetWorldRotation(Entity* entity, const quat& rotation)
{
    auto impl = Impl(entity);
    if (impl->parent)
        impl->local_rotation = quat_cast(GetWorldToLocal(impl->parent)) * rotation;
    else
        impl->local_rotation = rotation;

    MarkDirty(impl, false);
}

void SetWorldEulerAngles(Entity* entity, const vec3& angles)
{
    SetWorldRotation(entity, quat(radians(angles)));
}

vec3 TransformPoint(Entity* entity, const vec3& point)
{
    return vec3(GetLocalToWorld(entity) * vec4(point, 1.0f));
}

vec3 InverseTransformPoint(Entity* entity, const vec3& point)
{
    return vec3(GetWorldToLocal(entity) * vec4(point, 1.0f));
}

vec3 TransformDirection(Entity* entity, const vec3& direction)
{
    return normalize(vec3(GetLocalToWorld(entity) * vec4(direction, 0.0f)));
}

vec3 InverseTransformDirection(Entity* entity, const vec3& direction)
{
    return normalize(vec3(GetWorldToLocal(entity) * vec4(direction, 0.0f)));
}

void LookAt(Entity* entity, const vec3& target, const vec3& up)
{
    auto impl = Impl(entity);
    auto direction = normalize(target - impl->local_position);

    // Handle case where direction is parallel to up vector
    if (abs(dot(direction, up)) > 0.999f)
        SetWorldRotation(entity, quatLookAt(direction, abs(up.y) > 0.999f ? vec3(1, 0, 0) : vec3(0, 1, 0)));
    else
        SetWorldRotation(entity, quatLookAt(direction, up));
}

void RemoveFromParent(EntityImpl* impl, bool should_mark_dirty)
{
    assert(impl);

    if (!impl->parent)
        return;

    auto parent_impl = Impl(impl->parent);
    impl->parent = nullptr;
    Remove(parent_impl->children, impl);

    if (should_mark_dirty)
        MarkDirty(impl, true);
}

void RemoveFromParent(Entity* entity)
{
    RemoveFromParent(Impl(entity), true);
}

void SetParent(Entity* entity, Entity* parent)
{
    auto impl = Impl(entity);
    if (impl->parent == parent)
        return;

    if (impl->parent)
        RemoveFromParent(impl, false);

    impl->parent = parent;

    if (parent)
    {
        auto parent_impl = Impl(parent);
        impl->enabled_in_hierarchy = IsEnabled(parent);
        PushBack(parent_impl->children, impl);
    }

    UpdateRenderEntity(entity);
    MarkDirty(impl, true);
}

int GetChildCount(Entity* entity)
{
    return GetCount(Impl(entity)->children);;
}

Entity* GetFirstChild(Entity* entity)
{
    return (Entity*)GetFront(Impl(entity)->children);
}

Entity* GetNextChild(Entity* entity, Entity* child)
{
    return (Entity*)GetNext(Impl(entity)->children, child);
}

Entity* GetPrevChild(Entity* entity, Entity* child)
{
    return (Entity*)GetPrev(Impl(entity)->children, child);
}

static void SetEnabledInHierarchy(Entity* entity, bool enabled)
{
    auto impl = Impl(entity);
    if (enabled == impl->enabled_in_hierarchy)
        return;

    impl->enabled_in_hierarchy = enabled;

    UpdateRenderEntity(entity);

    // Recursively propagate the enabled state.
    enabled &= impl->enabled;
    for (auto child=GetFirstChild(entity); child; child=GetNextChild(entity, child))
        SetEnabledInHierarchy(child, enabled);
}

void SetEnabled(Entity* entity, bool enabled)
{
    auto impl = Impl(entity);
    impl->enabled = enabled;
    UpdateRenderEntity(entity);

    bool enabled_in_hierarchy = enabled && impl->enabled_in_hierarchy;
    for (auto child=GetFirstChild(entity); child; child=GetNextChild(entity, child))
        SetEnabledInHierarchy(child, enabled_in_hierarchy);
}

bool IsEnabled(Entity* entity)
{
    auto impl = Impl(entity);
    return impl->enabled_in_hierarchy && impl->enabled;
}

void BindTransform(Entity* entity)
{
    BindTransform(GetLocalToWorld(entity));
}

void RenderEntities(Camera* camera)
{
    for (auto entity = (Entity*)GetFront(g_render_entities); entity; entity = (Entity*)GetNext(g_render_entities, entity))
        GetEntityTraits(GetType(entity))->render(entity, camera);
}

Entity* CreateRootEntity(Allocator* allocator)
{
    auto entity = CreateEntity(allocator);
    auto impl = Impl(entity);
    impl->enabled_in_hierarchy = true;
    return entity;
}

Entity* CreateEntity(Allocator* allocator, size_t entity_size, type_t type_id)
{
    auto entity = (Entity*)CreateObject(allocator, entity_size, type_id, TYPE_ENTITY);
    auto impl = Impl(entity);
    impl->local_scale = VEC3_ONE;
    impl->local_rotation = identity<quat>();
    impl->enabled = true;
    impl->version = 1;
    Init(impl->children, offsetof(EntityImpl, node_child));
    MarkDirty(impl, false);
    SetParent(entity, GetSceneRoot());
    UpdateRenderEntity(entity);

    return entity;
}

Entity* CreateEntity(Allocator* allocator)
{
    return CreateEntity(allocator, sizeof(EntityImpl), TYPE_ENTITY);
}

void EntityDestructor(Object* o)
{
    auto entity = Impl((Entity*)o);
    auto traits = GetEntityTraits(GetType(o));
    if (traits->render)
        Remove(g_render_entities, o);

    RemoveFromParent(entity, false);
}

void InitEntity()
{
    static TypeTraits entity_type_traits = {
        .destructor = EntityDestructor,
    };
    SetTypeTraits(TYPE_ENTITY, &entity_type_traits);

    // To prevent the need for nullptr checks just initialize all to default
    static EntityTraits default_entity_traits = {};
    for (int i=0; i<TYPE_COUNT; i++)
        g_entity_traits[i] = &default_entity_traits;

    Init(g_render_entities, offsetof(EntityImpl, node_render));
}
//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if 0

void InitMeshRenderer();
void InitComponentList(LinkedList& list);
void SetEntity(Component* component, Entity* entity);
void RenderWorldCanvases(Camera* camera);

struct EntityFlags
{
    u32 value;
} __attribute__((packed));

constexpr u32 ENTITY_FLAG_ENABLED = 1 << 0;
constexpr u32 ENTITY_FLAG_ENABLED_SELF = 1 << 1;
constexpr u32 ENTITY_FLAG_ENABLED_HIERARCHY = 1 << 2;
constexpr u32 ENTITY_FLAG_LOCAL_DIRTY = 1 << 3;
constexpr u32 ENTITY_FLAG_WORLD_DIRTY = 1 << 4;

struct EntityImpl : Object
{
    const Name* name;
    Entity* parent;
    vec3 local_position;
    vec3 local_scale;
    quat local_rotation;
    mat4 local_to_world;
    mat4 world_to_local;
    LinkedList children;
    LinkedList components;
    LinkedListNode node_child;
    EntityFlags flags;
    u32 version;
    // const entity_traits* traits = nullptr;
};

static_assert(ENTITY_BASE_SIZE == sizeof(EntityImpl));

const EntityTraits* g_entity_traits[TYPE_COUNT] = {};
static Entity* g_root_entity = nullptr;
static Camera* g_screen_camera = nullptr;

void SetEntityTraits(type_t id, const EntityTraits* traits)
{
    g_entity_traits[id] = traits;
}

static bool IsEnabledInHierarchy(const EntityImpl* impl) { return !!(impl->flags.value & ENTITY_FLAG_ENABLED_HIERARCHY); }
static bool IsEnabledSelf(const EntityImpl* impl) { return !!(impl->flags.value & ENTITY_FLAG_ENABLED_SELF); }
static bool IsEnabled(const EntityImpl* impl) { return !!(impl->flags.value & ENTITY_FLAG_ENABLED); }
static bool IsLocalDirty(const EntityImpl* impl) { return !!(impl->flags.value & ENTITY_FLAG_LOCAL_DIRTY); }
static bool IsWorldDirty(const EntityImpl* impl) { return !!(impl->flags.value & ENTITY_FLAG_WORLD_DIRTY); }

static EntityImpl* Impl(Entity* e) { return (EntityImpl*)CastToBase(e, TYPE_ENTITY); }

static void UpdateComponentEnabled(EntityImpl* impl)
{
    if (IsEnabled(impl))
    {
        for (auto component=(Component*)GetFront(impl->components); component; component=(Component*)GetNext(impl->components, component))
            if (auto func = GetComponentTraits(GetType(component))->on_enabled)
                func(component);
    }
    else
    {
        for (auto component=(Component*)GetFront(impl->components); component; component=(Component*)GetNext(impl->components, component))
            if (auto func = GetComponentTraits(GetType(component))->on_disabled)
                func(component);
    }
}


static void UpdateEnabled(Entity* entity)
{
    auto impl = Impl(entity);
    auto was_enabled = IsEnabled(impl);
    auto is_enabled = IsEnabledInHierarchy(impl) && IsEnabledSelf(impl);
    if (was_enabled == is_enabled)
        return;

    if (is_enabled)
        impl->flags.value |= ENTITY_FLAG_ENABLED;
    else
        impl->flags.value &= ~ENTITY_FLAG_ENABLED;

    auto traits = GetEntityTraits(entity);
    if (is_enabled)
    {
        if (traits->on_enabled)
            traits->on_enabled(entity);

        UpdateComponentEnabled(impl);
    }
    else
    {
        UpdateComponentEnabled(impl);

        if (traits->on_disabled)
            traits->on_disabled(entity);
    }

    for (auto child=GetFirstChild(entity); child; child=GetNextChild(entity, child))
    {
        if (is_enabled)
            Impl(child)->flags.value |= ENTITY_FLAG_ENABLED_HIERARCHY;
        else
            Impl(child)->flags.value &= ~ENTITY_FLAG_ENABLED_HIERARCHY;

        UpdateEnabled(child);
    }
}

static void MarkDirty(EntityImpl* impl, bool force)
{
    assert(impl);

    if (!force && IsLocalDirty(impl))
        return;

    impl->flags.value |= (ENTITY_FLAG_LOCAL_DIRTY | ENTITY_FLAG_WORLD_DIRTY);
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

    impl->flags.value &= ~ENTITY_FLAG_LOCAL_DIRTY;
    impl->flags.value |= ENTITY_FLAG_WORLD_DIRTY;
}

static void UpdateWorldToLocal(EntityImpl* impl)
{
    if (IsLocalDirty(impl))
        UpdateLocalToWorld(impl);

    impl->world_to_local = inverse(impl->local_to_world);
    impl->flags.value &= ~ENTITY_FLAG_WORLD_DIRTY;
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
    if (IsWorldDirty(impl))
        UpdateWorldToLocal(impl);

    return impl->world_to_local;
}

const mat4& GetLocalToWorld(Entity* entity)
{
    EntityImpl* impl = Impl(entity);
    if (IsWorldDirty(impl))
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

    impl->flags.value &= ~ENTITY_FLAG_ENABLED_HIERARCHY;
    UpdateEnabled((Entity*)impl);

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
        if (IsEnabled(parent))
        {
            impl->flags.value |= ENTITY_FLAG_ENABLED_HIERARCHY;
            UpdateEnabled(entity);
        }

        PushBack(Impl(parent)->children, impl);
    }

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

void SetEnabled(Entity* entity, bool enabled)
{
    auto impl = Impl(entity);

    if (enabled == IsEnabledSelf(impl))
        return;

    if (!enabled)
        impl->flags.value &= ~ENTITY_FLAG_ENABLED_SELF;
    else
        impl->flags.value |= ENTITY_FLAG_ENABLED_SELF;

    UpdateEnabled(entity);
}

bool IsEnabled(Entity* entity)
{
    return IsEnabled(Impl(entity));
}

void BindTransform(Entity* entity)
{
    BindTransform(GetLocalToWorld(entity));
}

void AddComponent(Entity* entity, Component* component)
{
    auto impl = Impl(entity);
    assert(!IsInList(impl->components, component));
    PushBack(impl->components, component);
    SetEntity(component, entity);

    // If the entity is enabled, then call the component on_enabled
    if (IsEnabled(entity))
        UpdateComponentEnabled(impl);
}

void RemoveComponent(Entity* entity, Component* component)
{
    auto impl = Impl(entity);
    assert(IsInList(impl->components, component));

    // Call the disabled func always when removed
    if (IsEnabled(entity))
        if (auto func = GetComponentTraits(GetType(component))->on_disabled)
            func(component);

    Remove(impl->components, component);
    SetEntity(component, nullptr);
}

Component* GetComponent(Entity* entity, type_t type_id)
{
    auto impl = Impl(entity);
    for (auto component=(Component*)GetFront(impl->components); component; component=(Component*)GetNext(impl->components, component))
        if (GetType(component) == type_id)
            return component;

    return nullptr;
}

Entity* GetRootEntity()
{
    return g_root_entity;
}

static Entity* CreateRootEntity(Allocator* allocator)
{
    auto entity = CreateEntity(allocator);
    auto impl = Impl(entity);
    impl->flags.value =
        ENTITY_FLAG_LOCAL_DIRTY |
        ENTITY_FLAG_WORLD_DIRTY |
        ENTITY_FLAG_ENABLED |
        ENTITY_FLAG_ENABLED_SELF |
        ENTITY_FLAG_ENABLED_HIERARCHY;
    return entity;
}

Entity* CreateEntity(Allocator* allocator, size_t entity_size, type_t type_id)
{
    auto entity = (Entity*)CreateObject(allocator, entity_size, type_id, TYPE_ENTITY);
    auto impl = Impl(entity);
    impl->local_scale = VEC3_ONE;
    impl->local_rotation = identity<quat>();
    impl->flags.value =
        ENTITY_FLAG_LOCAL_DIRTY |
        ENTITY_FLAG_WORLD_DIRTY |
        ENTITY_FLAG_ENABLED_SELF;
    impl->version = 1;
    Init(impl->children, offsetof(EntityImpl, node_child));
    InitComponentList(impl->components);
    MarkDirty(impl, false);
    return entity;
}

Entity* CreateEntity(Allocator* allocator)
{
    return CreateEntity(allocator, sizeof(EntityImpl), TYPE_ENTITY);
}

void EntityDestructor(Object* o)
{
    auto impl = Impl((Entity*)o);
    while (auto component = (Component*)GetFront(impl->components))
    {
        Remove(impl->components, component);
        Destroy(component);
    }

    RemoveFromParent(impl, false);
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

    g_root_entity = CreateRootEntity(ALLOCATOR_DEFAULT);

    // components
    InitMeshRenderer();
}

void ShutdownEntity()
{
}

#ifdef NOZ_EDITOR
void WriteInspectorEntity(Stream* stream, Entity* entity)
{
    auto impl = Impl(entity);
    BeginInspectorObject(stream, GetType(entity), entity == GetRootEntity() ? "root" : "entity");
    WriteInspectorProperty(stream, "enabled", IsEnabled(entity));
    WriteInspectorProperty(stream, "local_position", impl->local_position);

    if (auto traits = GetEntityTraits(entity); traits->editor_inspect)
        traits->editor_inspect(entity, stream);

    for (auto child=GetFirstChild(entity); child; child=GetNextChild(entity, child))
        WriteInspectorEntity(stream, child);

    EndInspectorObject(stream);
}
#endif

#endif
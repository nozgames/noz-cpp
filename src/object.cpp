//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct ObjectImpl
{
    // todo: add a debug magic number here to validate object integrity
    type_t type;
    type_t base_type;
    uint32_t size;
    Allocator* allocator;
};

static_assert(OBJECT_BASE_SIZE == sizeof(ObjectImpl));
static_assert(OBJECT_OFFSET_TYPE == offsetof(ObjectImpl, type));
static_assert(OBJECT_OFFSET_BASE == offsetof(ObjectImpl, base_type));
static_assert(OBJECT_OFFSET_SIZE == offsetof(ObjectImpl, size));
static_assert(OBJECT_OFFSET_ALLOCATOR == offsetof(ObjectImpl, allocator));

static ObjectImpl* Impl(Object* object) { return (ObjectImpl*)object; }

static void ObjectDestructor(void* o)
{
    auto impl = (ObjectImpl*)o;

    // Call destructor if it has one
    auto type_traits = GetTypeTraits(impl->type);
    if (type_traits->destructor)
        type_traits->destructor((Object*)o);

    // Call base destructor if it has one
    type_traits = GetTypeTraits(impl->base_type);
    if (type_traits->destructor)
        type_traits->destructor((Object*)o);
}

Object* CreateObject(Allocator* allocator, size_t object_size, type_t object_type, type_t base_type)
{
    ObjectImpl* impl = Impl((Object*)Alloc(allocator, object_size, ObjectDestructor));
    if (!impl)
        return nullptr;

    impl->type = object_type;
    impl->base_type = base_type;
    impl->allocator = allocator;
    impl->size = (uint32_t)object_size;
    return (Object*)impl;
}

void Destroy(Object* o)
{
    if (!o)
        return;
    // todo: we would need to know the allocator to free this...  we could store it in the impl struct
}

//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct EventRegistry
{
    u32 listener_count;
};

struct EventListener
{
    EventCallback callback;
};

static u8* g_events = nullptr;
static u32 g_event_stride = 0;
static u32 g_max_events = 0;
static u32 g_max_listeners = 0;
static EventListener* g_event_stack = nullptr;
static u32 g_event_stack_size = 0;
static u32 g_event_max_stack_size = 0;

static int GetEventIndex(EventId event)
{
    assert(event + MAX_CORE_EVENTS >= 0);
    assert(event + MAX_CORE_EVENTS < g_max_events);
    return event + MAX_CORE_EVENTS;
}

static EventRegistry* GetRegistry(EventId event)
{
    return (EventRegistry*)(g_events + g_event_stride * GetEventIndex(event));
}

static EventListener* GetListener(EventId event, size_t index)
{
    assert(index < g_max_listeners);
    return (EventListener*)(GetRegistry(event) + 1) + index;
}

void InitEvent(ApplicationTraits* traits)
{
    g_max_listeners = traits->max_event_listeners;
    g_event_stride = sizeof(EventListener) * g_max_listeners + sizeof(EventRegistry);
    g_max_events = traits->max_events + MAX_CORE_EVENTS;
    g_event_max_stack_size = traits->max_event_stack;
    assert(g_event_stride > 0);
    assert(g_max_events > 0);
    g_events = (u8*)Alloc(ALLOCATOR_DEFAULT, g_event_stride * g_max_events);
    g_event_stack = (EventListener*)Alloc(ALLOCATOR_DEFAULT, (u32)sizeof(EventListener) * traits->max_event_stack);
}

void ShutdownEvent()
{
    Free(g_events);
    Free(g_event_stack);
}

int FindListener(EventId event, EventCallback callback)
{
    auto registry = GetRegistry(event);
    auto listener = GetListener(event, 0);
    for (u32 i = 0; i < registry->listener_count; ++i, listener++)
        if (listener->callback == callback)
            return i;
    return -1;
}

void Listen(EventId event, EventCallback callback)
{
    assert(-1 == FindListener(event, callback));
    auto registry = GetRegistry(event);
    if (registry->listener_count >= g_max_listeners)
    {
        ExitOutOfMemory("max_event_listeners");
        return;
    }

    auto listener = GetListener(event, registry->listener_count++);
    listener->callback = callback;
}

void Unlisten(EventId event, EventCallback callback)
{
    int listener_index = FindListener(event, callback);
    if (listener_index == -1)
        return;

    // Move all the listeners down to fill the gap but maintain their order
    auto registry = GetRegistry(event);
    for (size_t i = listener_index; i < registry->listener_count - 1; ++i)
        GetListener(event, i)->callback = GetListener(event, i + 1)->callback;

    registry->listener_count--;
}

void Send(EventId event, const void* event_data)
{
    auto listener_count = GetRegistry(event)->listener_count;
    if (g_event_stack_size + listener_count > g_event_max_stack_size)
    {
        ExitOutOfMemory("max_event_stack");
        return;
    }

    auto stack_head_index = g_event_stack_size;
    g_event_stack_size += listener_count;

    auto stack_head = g_event_stack + stack_head_index;
    memcpy(stack_head, GetListener(event, 0), sizeof(EventListener) * listener_count);

    auto listener = stack_head;
    for (size_t i=0; i<listener_count; ++i, listener++)
    {
        assert(listener->callback);
        listener->callback(event, event_data);
    }

    g_event_stack_size = stack_head_index;
}

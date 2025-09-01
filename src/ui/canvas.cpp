//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#if 0

Element* CreateRootElement(Allocator* allocator, Canvas* canvas, const name_t* id);
void WriteInspectorElement(Stream* stream, Element* element);
void RenderElements(Element* element, const mat4& canvas_transform, const vec2& canvas_size, bool is_dirty);
void BeginUIPass();

struct CanvasImpl : Entity
{
    Element* root;
    StyleSheet* style_sheet;
    vec2 size;
    ivec2 screen_size;
    vec2 reference_size;
    CanvasType type;
    LinkedListNode node_render;
    bool dirty;
};

static LinkedList g_screen_render = {};
static LinkedList g_world_render = {};

static CanvasImpl* Impl(Canvas* c) { return (CanvasImpl*)Cast(c, TYPE_CANVAS);}

void MarkDirty(Canvas* canvas)
{
    if (!canvas)
        return;

    Impl(canvas)->dirty = true;
}

Element* GetRootElement(Canvas* canvas)
{
    if (!canvas)
        return nullptr;

    return Impl(canvas)->root;
}

bool IsVisible(Canvas* canvas)
{
    return IsVisible(Impl(canvas)->root);
}

void SetVisible(Canvas* canvas, bool value)
{
    SetVisible(Impl(canvas)->root, value);
}

StyleSheet* GetStyleSheet(Canvas* canvas)
{
    return Impl(canvas)->style_sheet;
}

void SetStyleSheet(Canvas* canvas, StyleSheet* sheet)
{
    Impl(canvas)->style_sheet = sheet;
    MarkDirty(canvas);
}

void AddChild(Canvas* canvas, Element* child)
{
    AddChild(GetRootElement(canvas), child);
}

void DrawScreenCanvases()
{
    auto screen_size = GetScreenSize();

    BeginUIPass();

    for (auto canvas = (Canvas*)GetFront(g_screen_render); canvas; canvas = (Canvas*)GetNext(g_screen_render, canvas))
    {
        auto impl = Impl(canvas);

        // Update screen size and canvas size if needed (same logic as before)
        if (impl->screen_size != screen_size)
        {
            impl->screen_size = screen_size;

            auto sw = static_cast<float>(screen_size.x);
            auto sh = static_cast<float>(screen_size.y);
            auto sw_rw = sw / impl->reference_size.x;
            auto sh_rh = sh / impl->reference_size.y;

            float ortho_width;
            float ortho_height;
            if (std::abs(sw_rw - 1.0f) < std::abs(sh_rh - 1.0f))
            {
                ortho_width = impl->reference_size.x;
                ortho_height = impl->reference_size.x * sh / sw;
            }
            else
            {
                ortho_height = impl->reference_size.y;
                ortho_width = impl->reference_size.y * sw / sh;
            }

            impl->size = vec2(ortho_width, ortho_height);
            impl->dirty = true;
        }

        // Render the screen canvas with UI camera
        static const mat4 identity = glm::identity<mat4>();
        auto p = ortho(0.0f, impl->size.x, impl->size.y, 0.0f, -1.0f, 1.0f);
        BindCamera(identity, p);
        RenderElements(impl->root, identity, impl->size, impl->dirty);
        impl->dirty = false;
    }

    EndRenderPass();
}

void DrawWorldCanvases(Camera* camera)
{
    auto transform = GetLocalToWorld(camera);
    for (auto canvas = (Canvas*)GetFront(g_world_render); canvas; canvas = (Canvas*)GetNext(g_world_render, canvas))
    {
        auto impl = Impl(canvas);
        auto dirty = impl->dirty;
        impl->dirty = false;
        RenderElements(impl->root, transform, impl->size, dirty);
    }
}

void CanvasOnEnabled(Entity* entity)
{
    auto canvas = (Canvas*)entity;
    auto impl = Impl(canvas);
    if (impl->type == CANVAS_TYPE_WORLD)
        PushBack(g_world_render, entity);
    else
        PushBack(g_screen_render, entity);
}

void CanvasOnDisabled(Entity* entity)
{
    auto impl = Impl((Canvas*)entity);
    if (impl->type == CANVAS_TYPE_WORLD)
        Remove(g_world_render, entity);
    else
        Remove(g_screen_render, entity);
}

Canvas* CreateCanvas(Allocator* allocator, CanvasType type, float reference_width, float reference_height, const name_t* id)
{
    auto canvas = (Canvas*)CreateEntity(allocator, sizeof(CanvasImpl), TYPE_CANVAS);
    auto impl = Impl(canvas);
    impl->root = CreateRootElement(allocator, canvas, id);
    impl->reference_size = vec2(reference_width, reference_height);
    impl->size = impl->reference_size;
    impl->dirty = true;
    impl->type = type;
    return canvas;
}

#ifdef NOZ_EDITOR
void CanvasEditorInspect(Entity* entity, Stream* stream)
{
    auto impl = Impl((Canvas*)entity);
    WriteInspectorProperty(stream, "sheet", impl->style_sheet ? GetName(impl->style_sheet)->value : "null");
    WriteInspectorElement(stream, impl->root);
}
#endif

void InitCanvas()
{
    static EntityTraits traits = {
        .on_enabled = CanvasOnEnabled,
        .on_disabled = CanvasOnDisabled,
#ifdef NOZ_EDITOR
        .editor_inspect = CanvasEditorInspect
#endif
    };

    SetEntityTraits(TYPE_CANVAS, &traits);

    Init(g_screen_render, offsetof(CanvasImpl, node_render));
    Init(g_world_render, offsetof(CanvasImpl, node_render));
}

#endif
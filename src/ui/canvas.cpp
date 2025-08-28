//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

Element* CreateRootElement(Allocator* allocator, Canvas* canvas, const name_t* id);
void RenderElements(Element* element, const mat4& canvas_transform, const vec2& canvas_size, bool is_dirty);

struct CanvasImpl
{
    ENTITY_BASE;
    Element* root;
    StyleSheet* style_sheet;
    vec2 size;
    ivec2 screen_size;
    vec2 reference_size;
    CanvasType type;
    LinkedListNode node_screen_render;
    bool dirty;
};

static LinkedList g_screen_render = {};

static CanvasImpl* Impl(Canvas* c) { return (CanvasImpl*)Cast(c, TYPE_CANVAS);}

void MarkDirty(Canvas* canvas)
{
    if (!canvas)
        return;

    Impl(canvas)->dirty = true;
}

Element* GetRootElement(Canvas* canvas)
{
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

void RenderScreenCanvases()
{
    auto screen_size = GetScreenSize();

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
        auto p = glm::ortho(0.0f, impl->size.x, impl->size.y, 0.0f, -1.0f, 1.0f);
        BindCamera(identity, p);
        RenderElements(impl->root, identity, impl->size, impl->dirty);
        impl->dirty = false;
    }

    Clear(g_screen_render);
}

static void RenderWorldCanvas(CanvasImpl* impl, const mat4& transform, Camera* camera)
{
    auto dirty = impl->dirty;
    impl->dirty = false;
    RenderElements(impl->root, transform, impl->size, dirty);
}

static void CanvasRender(Entity* entity, Camera* camera)
{
    auto impl = Impl((Canvas*)entity);
    switch (impl->type)
    {
    case CANVAS_TYPE_SCREEN:
        PushBack(g_screen_render, entity);
        break;

    case CANVAS_TYPE_WORLD:
        RenderWorldCanvas(impl, GetLocalToWorld(entity), camera);
        break;
    }
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

void InitCanvas()
{
    Init(g_screen_render, offsetof(CanvasImpl, node_screen_render));
}
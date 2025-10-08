//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr int MAX_ELEMENTS = 4096;
constexpr int MAX_ELEMENT_STACK = 128;
constexpr int MAX_TEXT_MESHES = 256;

extern void UpdateInputState(InputSet* input_set);

enum ElementType : u8 {
    ELEMENT_TYPE_UNKNOWN = 0,
    ELEMENT_TYPE_NONE,
    ELEMENT_TYPE_BORDER,
    ELEMENT_TYPE_CANVAS,
    ELEMENT_TYPE_COLUMN,
    ELEMENT_TYPE_CONTAINER,
    ELEMENT_TYPE_EXPANDED,
    ELEMENT_TYPE_GESTURE_DETECTOR,
    ELEMENT_TYPE_IMAGE,
    ELEMENT_TYPE_INSET,
    ELEMENT_TYPE_LABEL,
    ELEMENT_TYPE_MOUSE_REGION,
    ELEMENT_TYPE_RECTANGLE,
    ELEMENT_TYPE_ROW,
    ELEMENT_TYPE_SIZED_BOX,
    ELEMENT_TYPE_STACK,
    ELEMENT_TYPE_TRANSFORM,
    ELEMENT_TYPE_COUNT
};

struct CachedTextMesh
{
    TextMesh* text_mesh;
    u64 hash;
    int last_frame;
};

struct Element {
    ElementType type;
    ElementState state;
    u32 index;
    Rect rect;
    Rect rect_with_margins;
    u32 child_count;
    Vec2 measured_size;
    Mat3 local_to_world;
    Mat3 world_to_local;
};

struct BorderElement : Element {
    BorderStyle style;
};

struct CanvasElement : Element {
    CanvasStyle style;
};

struct ContainerElement : Element {
    ContainerStyle style;
};

struct ExpandedElement : Element {
    ExpandedStyle style;
};

struct GestureDetectorElement : Element {
    GestureDetectorStyle style;
};

struct MouseRegionElement : Element {
    MouseRegionStyle style;
};

struct RowElement : Element {
    RowStyle style;
};

struct ColumnElement : Element {
    ColumnStyle style;
};

struct LabelElement : Element {
    LabelStyle style;
    CachedTextMesh* cached_mesh = nullptr;
    Vec2 offset;
};

struct RectangleElement : Element {
    RectangleStyle style;
};

struct StackElement : Element {
};

struct ImageElement : Element {
    ImageStyle style;
    Material* material = nullptr;
    Mesh* mesh = nullptr;
};

struct InsetElement : Element {
    EdgeInsets insets;
};

struct TransformElement : Element {
    TransformStyle style;
};

union FatElement {
    Element element;
    BorderElement border;
    CanvasElement canvas;
    ColumnElement column;
    ContainerElement container;
    ExpandedElement expanded;
    GestureDetectorElement gesture_detector;
    LabelElement label;
    MouseRegionElement mouse_region;
    ImageElement image;
    InsetElement inset;
    RowElement row;
};

struct UI {
    Allocator* allocator;
    Camera* camera;
    Element* elements[MAX_ELEMENTS];
    Element* element_stack[MAX_ELEMENTS];
    u32 element_stack_count;
    Mesh* element_quad;
    u32 element_count;
    Vec2 ortho_size;
    Vec2Int ref_size;
    Material* element_material;
    InputSet* input;
    PoolAllocator* text_mesh_allocator;
};

static UI g_ui = {};

static Element* CreateElement(ElementType type) {
    Element* element = static_cast<Element*>(Alloc(g_ui.allocator, sizeof(FatElement)));
    element->type = type;
    element->index= g_ui.element_count;
    g_ui.elements[g_ui.element_count++] = element;
    return element;
}

static void PushElement(Element* element) {
    if (g_ui.element_stack_count >= MAX_ELEMENT_STACK)
        return;

    g_ui.element_stack[g_ui.element_stack_count++] = element;
}

static void PopElement() {
    if (g_ui.element_stack_count == 0)
        return;

    g_ui.element_stack_count--;
}

static void IncrementChildCount() {
    assert(g_ui.element_stack_count > 0);
    g_ui.element_stack[g_ui.element_stack_count-1]->child_count++;
}

static void ExecuteChildren(Element* element, void (*children)()) {
    if (!children)
        return;

    PushElement(element);
    children();
    PopElement();
}

void Canvas(const CanvasStyle& style, void (*children)()) {
    CanvasElement* canvas = static_cast<CanvasElement*>(CreateElement(ELEMENT_TYPE_CANVAS));
    canvas->style = style;
    ExecuteChildren(canvas, children);
}

void Row(const RowStyle& style, void (*children)()) {
    IncrementChildCount();
    RowElement* row = static_cast<RowElement*>(CreateElement(ELEMENT_TYPE_ROW));
    row->style = style;
    ExecuteChildren(row, children);
}

void Column(const ColumnStyle& style, void (*children)()) {
    IncrementChildCount();
    ColumnElement* column = static_cast<ColumnElement*>(CreateElement(ELEMENT_TYPE_COLUMN));
    column->style = style;
    ExecuteChildren(column, children);
}

void Stack(void (*children)()) {
    IncrementChildCount();
    StackElement* stack = static_cast<StackElement*>(CreateElement(ELEMENT_TYPE_STACK));
    ExecuteChildren(stack, children);
}

void Container(const ContainerStyle& style, void (*children)()) {
    IncrementChildCount();
    ContainerElement* container = static_cast<ContainerElement*>(CreateElement(ELEMENT_TYPE_CONTAINER));
    container->style = style;
    ExecuteChildren(container, children);
}

void Expanded(const ExpandedStyle& style, void (*children)()) {
    IncrementChildCount();
    ExpandedElement* e = static_cast<ExpandedElement*>(CreateElement(ELEMENT_TYPE_EXPANDED));
    e->style = style;
    ExecuteChildren(e, children);
}

void GestureDetector(const GestureDetectorStyle& style, void (*children)()) {
    IncrementChildCount();
    GestureDetectorElement* gesture_detector = static_cast<GestureDetectorElement*>(CreateElement(ELEMENT_TYPE_GESTURE_DETECTOR));
    gesture_detector->style = style;
    ExecuteChildren(gesture_detector, children);
}

static u64 GetMeshHash(const TextRequest& request)
{
    return Hash(Hash(request.text), reinterpret_cast<u64>(request.font), static_cast<u64>(request.font_size));
}

static CachedTextMesh* GetOrCreateTextMesh(const char* text, const LabelStyle& style) {
    TextRequest r = {};
    r.font = style.font;
    r.font_size = style.font_size;
    SetValue(r.text, text);

    struct EnumArgs {
        u64 hash;
        CachedTextMesh* result;
    };

    EnumArgs args = { GetMeshHash(r), nullptr };
    Enumerate(g_ui.text_mesh_allocator, [](u32, void* item_ptr, void* user_data) {
        CachedTextMesh* c = static_cast<CachedTextMesh*>(item_ptr);
        EnumArgs* a = static_cast<EnumArgs*>(user_data);
        if (c->hash != a->hash)
            return true;

        a->result = c;
        return false;
    }, &args);

    if (!args.result) {
        if (TextMesh* tm = CreateTextMesh(ALLOCATOR_DEFAULT, r))
        {
            args.result = static_cast<CachedTextMesh*>(Alloc(g_ui.text_mesh_allocator, sizeof(CachedTextMesh)));
            args.result->hash = args.hash;
            args.result->text_mesh = tm;
            args.result->last_frame = 0;
        }
    }

    return args.result;
}

void Label(const char* text, const LabelStyle& style) {
    IncrementChildCount();
    LabelElement* label = static_cast<LabelElement*>(CreateElement(ELEMENT_TYPE_LABEL));
    label->style = style;
    label->cached_mesh = GetOrCreateTextMesh(text, style);
    ExecuteChildren(label, nullptr);
}

void Inset(float amount, void (*children)()) {
    IncrementChildCount();
    InsetElement* inset = static_cast<InsetElement*>(CreateElement(ELEMENT_TYPE_INSET));
    inset->insets = amount;
    ExecuteChildren(inset, children);
}

void Inset(const EdgeInsets& insets, void (*children)()) {
    IncrementChildCount();
    InsetElement* inset = static_cast<InsetElement*>(CreateElement(ELEMENT_TYPE_INSET));
    inset->insets = insets;
    ExecuteChildren(inset, children);
}

void Image(Material* material, const ImageStyle& style) {
    IncrementChildCount();
    ImageElement* image = static_cast<ImageElement*>(CreateElement(ELEMENT_TYPE_IMAGE));
    image->material = material;
    image->mesh = g_ui.element_quad;
    image->style = style;
}

void Image(Material* material, Mesh* mesh, const ImageStyle& style) {
    IncrementChildCount();
    ImageElement* image = static_cast<ImageElement*>(CreateElement(ELEMENT_TYPE_IMAGE));
    image->material = material;
    image->mesh = mesh;
    image->style = style;
}

void MouseRegion(const MouseRegionStyle& style, void (*children)()) {
    IncrementChildCount();
    MouseRegionElement* mouse_region = static_cast<MouseRegionElement*>(CreateElement(ELEMENT_TYPE_MOUSE_REGION));
    mouse_region->style = style;
    ExecuteChildren(mouse_region, children);
}

void Rectangle(const RectangleStyle& style) {
    IncrementChildCount();
    RectangleElement* rectangle = static_cast<RectangleElement*>(CreateElement(ELEMENT_TYPE_RECTANGLE));
    rectangle->style = style;
}

void Transformed(const TransformStyle& style, void (*children)()) {
    IncrementChildCount();
    TransformElement* transform = static_cast<TransformElement*>(CreateElement(ELEMENT_TYPE_TRANSFORM));
    transform->style = style;
    ExecuteChildren(transform, children);
}

#if 0
static int MeasureElement(int element_index, const Vec2& available_size) {
    Element* e = g_ui.elements[element_index++];

    Vec2 adjusted_size = available_size;

    if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* c = static_cast<ContainerElement*>(e);
        adjusted_size.x -= (c->style.padding.left + c->style.padding.right);
        adjusted_size.y -= (c->style.padding.top + c->style.padding.bottom);
    } else if (e->type == ELEMENT_TYPE_INSET) {
        InsetElement* inset = static_cast<InsetElement*>(e);
        adjusted_size.x -= (inset->insets.left + inset->insets.right);
        adjusted_size.y -= (inset->insets.top + inset->insets.bottom);
    } else if (e->type == ELEMENT_TYPE_ROW) {
        RowElement* row = static_cast<RowElement*>(e);
        adjusted_size.x -= row->style.spacing * Max(0, static_cast<int>(e->child_count) - 1);
    } else if (e->type == ELEMENT_TYPE_COLUMN) {
        ColumnElement* column = static_cast<ColumnElement*>(e);
        adjusted_size.y -= column->style.spacing * Max(0, static_cast<int>(e->child_count) - 1);
    } else if (e->type == ELEMENT_TYPE_LABEL) {
        LabelElement* l = static_cast<LabelElement*>(e);
        if (l->cached_mesh) {
            Vec2 text_size = GetSize(l->cached_mesh->text_mesh);
            e->measured_size = {
                l->style.align == TEXT_ALIGN_MIN ? text_size.x : adjusted_size.x,
                l->style.vertical_align == TEXT_ALIGN_MIN ? text_size.y : adjusted_size.y
            };
        } else {
            e->measured_size = {0,0};
        }
    } else if (e->type == ELEMENT_TYPE_IMAGE) {
        ImageElement* i = static_cast<ImageElement*>(e);
        if (i->mesh) {
            Vec2 mesh_size = GetSize(i->mesh);
            e->measured_size = {
                Min(mesh_size.x, adjusted_size.x),
                Min(mesh_size.y, adjusted_size.y)
            };
        } else {
            e->measured_size = {0,0};
        }
        return element_index;
    } else if (e->type == ELEMENT_TYPE_EXPANDED) {
        e->measured_size = adjusted_size;
    }

    for (u32 child_index = 0; child_index < e->child_count; child_index++)
        element_index = MeasureElement(element_index, adjusted_size);

    return element_index;
}
#endif

static int LayoutElement(int element_index, const Rect& rect);

static int SkipElement(int element_index) {
    Element* e = g_ui.elements[element_index++];
    for (u32 i = 0; i < e->child_count; i++)
        element_index = SkipElement(element_index);
    return element_index;
}

static int LayoutChildren(int element_index, Element* parent, const Rect& content_rect) {
    Rect available_rect = content_rect;

    u32 flex_element_count = 0;
    float flex_total = 0.0f;
    u32 element_stack_start = g_ui.element_stack_count;
    for (u32 i = 0; i < parent->child_count; i++) {
        Element* child = g_ui.elements[element_index++];
        g_ui.element_stack[g_ui.element_stack_count++] = child;

        if (child->type == ELEMENT_TYPE_EXPANDED) {
            flex_element_count++;
            flex_total += static_cast<ExpandedElement*>(child)->style.flex;
            element_index = SkipElement(element_index - 1);
            continue;
        }

        element_index = LayoutElement(element_index - 1, available_rect);

        if (parent->type == ELEMENT_TYPE_ROW) {
            RowElement* row = static_cast<RowElement*>(parent);
            available_rect.x += child->rect_with_margins.width + row->style.spacing;
            available_rect.width -= child->rect_with_margins.width;
        } else if (parent->type == ELEMENT_TYPE_COLUMN) {
            ColumnElement* column = static_cast<ColumnElement*>(parent);
            available_rect.y += child->rect_with_margins.height + column->style.spacing;
            available_rect.height -= child->rect_with_margins.height;
        }
    }

    if (flex_element_count > 0 && parent->type == ELEMENT_TYPE_ROW) {
        float offset = 0.0f;
        for (u32 i = 0; i < parent->child_count; i++) {
            Element* child = g_ui.element_stack[element_stack_start + i];
            if (child->type != ELEMENT_TYPE_EXPANDED) {
                child->rect.x += offset;
                available_rect.x = child->rect.x + child->rect.width;
                continue;
            }

            ExpandedElement* expanded = static_cast<ExpandedElement*>(child);
            Rect flex_rect = available_rect;
            flex_rect.width = available_rect.width * (expanded->style.flex / flex_total);
            LayoutElement(child->index, available_rect);

            available_rect.x += flex_rect.width;
            offset += flex_rect.width;
        }
    } else if (flex_element_count > 0 && parent->type == ELEMENT_TYPE_COLUMN) {
        float offset = 0.0f;
        for (u32 i = 0; i < parent->child_count; i++) {
            Element* child = g_ui.element_stack[element_stack_start + i];
            if (child->type != ELEMENT_TYPE_EXPANDED) {
                child->rect.y += offset;
                available_rect.y = child->rect.y + child->rect.height;
                continue;
            }

            ExpandedElement* expanded = static_cast<ExpandedElement*>(child);
            Rect flex_rect = available_rect;
            flex_rect.height = available_rect.height * (expanded->style.flex / flex_total);
            LayoutElement(child->index, flex_rect);

            available_rect.y += flex_rect.height;
            offset += flex_rect.height;
        }
    }

    g_ui.element_stack_count-=parent->child_count;

    return element_index;
}

static int LayoutElement(int element_index, const Rect& rect) {
    Element* e = g_ui.elements[element_index++];
    assert(e);

    e->rect = rect;
    e->rect_with_margins = rect;

    if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        Vec2 margin = {
            container->style.margin.left + container->style.margin.right,
            container->style.margin.top + container->style.margin.bottom
        };
        e->rect.height = Min(container->style.height, e->rect.height - margin.y);
        e->rect.width = Min(container->style.width, e->rect.width - margin.x);
        e->rect.x += container->style.margin.left;
        e->rect.y += container->style.margin.top;
        e->rect_with_margins.width = e->rect.width + margin.x;
        e->rect_with_margins.height = e->rect.height + margin.y;
    }

    if (e->type == ELEMENT_TYPE_LABEL) {
        LabelElement* l = static_cast<LabelElement*>(e);
        Vec2 text_size = GetSize(l->cached_mesh->text_mesh);
        if (l->style.align == TEXT_ALIGN_MIN) e->rect.width = text_size.x;
        if (l->style.vertical_align == TEXT_ALIGN_MIN) e->rect.height = text_size.y;

        if (l->style.align == TEXT_ALIGN_CENTER) {
            l->offset.x = (e->rect.width - text_size.x) * 0.5f;
        } else if (l->style.align == TEXT_ALIGN_MAX) {
            l->offset.x = e->rect.width - text_size.x;
        }

        if (l->style.vertical_align == TEXT_ALIGN_CENTER) {
            l->offset.y = (e->rect.height - text_size.y) * 0.5f;
        } else if (l->style.vertical_align == TEXT_ALIGN_MAX) {
            l->offset.y = e->rect.height - text_size.y;
        }
    }

    if (e->child_count == 0)
        return element_index;

    Rect content_rect = {0, 0, e->rect.width, e->rect.height};
    if (e->type == ELEMENT_TYPE_INSET) {
        InsetElement* inset = static_cast<InsetElement*>(e);
        content_rect.x += inset->insets.left;
        content_rect.y += inset->insets.top;
        content_rect.width -= (inset->insets.left + inset->insets.right);
        content_rect.height -= (inset->insets.top + inset->insets.bottom);
    } else if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        content_rect.x += container->style.padding.left;
        content_rect.y += container->style.padding.top;
        content_rect.width -= (container->style.padding.left + container->style.padding.right);
        content_rect.height -= (container->style.padding.top + container->style.padding.bottom);
    }

    return LayoutChildren(element_index, e, content_rect);
}

static u32 CalculateTransforms(u32 element_index, const Mat3& parent_transform)
{
    Element* e = g_ui.elements[element_index++];

    if (e->type == ELEMENT_TYPE_TRANSFORM) {
        TransformElement* transform = static_cast<TransformElement*>(e);

        Vec2 pivot = {
            e->rect.width * transform->style.origin.x,
            e->rect.height * transform->style.origin.y
        };

        Mat3 local_transform =
            Translate({transform->style.translate.x + e->rect.x, transform->style.translate.y + e->rect.y}) *
            Translate(pivot) *
            TRS(VEC2_ZERO, transform->style.rotate, transform->style.scale) *
            Translate(-pivot);

        e->local_to_world = parent_transform * local_transform;
    } else {
        e->local_to_world = parent_transform * Translate({e->rect.x, e->rect.y});
    }

    e->world_to_local = Inverse(e->local_to_world);

    for (u32 i = 0; i < e->child_count; i++)
        element_index = CalculateTransforms(element_index, e->local_to_world);

    return element_index;
}


void RenderCanvas(Element* e)
{
    (void)e;
    UpdateCamera(g_ui.camera);
    BindCamera(g_ui.camera);
}

#if 0
static void RenderBorder(Element& e, const Mat3& transform)
{
    // float border_width = e.style.border_width.value;
    // BindColor(e.style.border_color.value);
    // BindMaterial(g_ui.element_material);
    // DrawMesh(
    //     g_ui.element_quad,
    //     transform * Scale(Vec2{e.rect.width, border_width}));
    // DrawMesh(
    //     g_ui.element_quad,
    //     transform
    //         * Translate(Vec2{0,e.rect.height - border_width})
    //         * Scale(Vec2{e.rect.width, border_width}));
    // DrawMesh(
    //     g_ui.element_quad,
    //     transform
    //         * Translate(Vec2{e.rect.width - border_width,border_width})
    //         * Scale(Vec2{border_width, e.rect.height - border_width * 2}));
    // DrawMesh(
    //     g_ui.element_quad,
    //     transform
    //         * Translate(Vec2{0, border_width})
    //         * Scale(Vec2{border_width, e.rect.height - border_width * 2}));
}
#endif

static void RenderBackground(const Rect& rect, const Mat3& transform, const Color& color)
{
    BindTransform(transform * Scale(Vec2{rect.width, rect.height}));
    BindMaterial(g_ui.element_material);
    BindColor(color);
    DrawMesh(g_ui.element_quad);
}

static int RenderElement(int element_index)
{
    Element* e = g_ui.elements[element_index++];
    const Mat3& transform = e->local_to_world;

    if (e->type == ELEMENT_TYPE_CANVAS)
        RenderCanvas(e);

    // if (e.style.border_width.value > F32_EPSILON && e.style.border_color.value.a > F32_EPSILON)
    //     RenderBorder(e, transform);
    //
    // if (e.style.background_color.value.a > 0)
    // {
    //     BindTransform(transform * Scale(Vec2{e.rect.width, e.rect.height}));
    //     BindMaterial(g_ui.element_material);
    //     BindColor(e.style.background_color.value);
    //     DrawMesh(g_ui.element_quad);
    // }

    // if (e.style.background_vignette_color.value.a > 0)
    // {
    //     VignetteBuffer vignette = {
    //         .intensity = e.style.background_vignette_intensity.value,
    //         .smoothness = e.style.background_vignette_smoothness.value
    //     };
    //
    //     BindTransform(
    //         {e.rect.x + e.style.translate_x.value, e.rect.y + e.style.translate_y.value},
    //         e.style.rotate.value,
    //         Vec2{e.rect.width, e.rect.height} * e.style.scale.value);
    //     BindMaterial(g_ui.vignette_material);
    //     BindColor(e.style.background_vignette_color.value);
    //     BindFragmentUserData(&vignette, sizeof(vignette));
    //     DrawMesh(g_ui.element_quad);
    // }

    if (e->type == ELEMENT_TYPE_LABEL) {
        LabelElement* l = static_cast<LabelElement*>(e);
        Mesh* mesh = l->cached_mesh ? GetMesh(l->cached_mesh->text_mesh) : nullptr;
        if (mesh) {
            BindTransform(transform * Translate(l->offset));
            BindColor(l->style.color);
            BindMaterial(GetMaterial(l->cached_mesh->text_mesh));
            DrawMesh(mesh);
        }
    } else if (e->type == ELEMENT_TYPE_IMAGE) {
        ImageElement* image = static_cast<ImageElement*>(e);
        BindMaterial(image->material);
        BindColor(image->style.color);
        BindTransform(transform * Scale({e->rect.width, e->rect.height}));
        DrawMesh(image->mesh);
    } else if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        RenderBackground(e->rect, transform, container->style.color);
    } else if (e->type == ELEMENT_TYPE_CANVAS) {
        CanvasElement* canvas = static_cast<CanvasElement*>(e);
        RenderBackground(e->rect, transform, canvas->style.color);
    } else if (e->type == ELEMENT_TYPE_RECTANGLE) {
        RectangleElement* rectangle = static_cast<RectangleElement*>(e);
        BindTransform(transform * Scale(Vec2{e->rect.width, e->rect.height}));
        BindMaterial(g_ui.element_material);
        if (rectangle->style.color_func) {
            BindColor(rectangle->style.color_func(e->state, 0.0f));
        } else {
            BindColor(rectangle->style.color);
        }
        DrawMesh(g_ui.element_quad);
    }

    for (u32 i = 0; i < e->child_count; i++)
        element_index = RenderElement(element_index);

    return element_index;
}

void BeginUI(u32 ref_width, u32 ref_height) {
    g_ui.ref_size = { (i32)ref_width, (i32)ref_height };
    g_ui.element_stack_count = 0;
    g_ui.element_count = 0;

    Clear(g_ui.allocator);

    //g_ui.in_frame = true;

    Vec2Int screen_size = GetScreenSize();

    f32 rw = (f32)g_ui.ref_size.x;
    f32 rh = (f32)g_ui.ref_size.y;
    f32 sw = (f32)screen_size.x;
    f32 sh = (f32)screen_size.y;
    f32 sw_rw = sw / rw;
    f32 sh_rh = sh / rh;

    if (Abs(sw_rw - 1.0f) < Abs(sh_rh - 1.0f))
    {
        g_ui.ortho_size.x = rw;
        g_ui.ortho_size.y = rw * sh / sw;
    }
    else
    {
        g_ui.ortho_size.y = rh;
        g_ui.ortho_size.x = rh * sw / sh;
    }

    SetExtents(g_ui.camera, 0, g_ui.ortho_size.x, 0, g_ui.ortho_size.y, false);

    UpdateInputState(g_ui.input);
}

static void HandleInput()
{
    Vec2 mouse = ScreenToWorld(g_ui.camera, GetMousePosition());
    for (u32 i=g_ui.element_count; i>0; i--)
    {
        Element* e = g_ui.elements[i-1];
        Vec2 local_mouse = TransformPoint(e->world_to_local, mouse);
        bool mouse_over = Contains(Bounds2{0,0,e->rect.width, e->rect.height}, local_mouse);

        if (mouse_over)
            e->state = e->state | ELEMENT_STATE_HOVERED;
        else
            e->state = e->state & ~ELEMENT_STATE_HOVERED;

        if (e->type == ELEMENT_TYPE_GESTURE_DETECTOR) {
            GestureDetectorElement* g = static_cast<GestureDetectorElement*>(e);
            if (mouse_over && g->style.on_tap && WasButtonPressed(g_ui.input, MOUSE_LEFT)) {
                g->style.on_tap();
                ConsumeButton(MOUSE_LEFT);
            }
        }
    }
}

void EndUI()
{
    Rect rect = {0, 0, g_ui.ortho_size.x, g_ui.ortho_size.y};
    // for (u32 element_index=0; element_index < g_ui.element_count; element_index++)
    //     element_index = MeasureElement(element_index, g_ui.ortho_size);
    for (u32 element_index=0; element_index < g_ui.element_count; element_index++)
        element_index = LayoutElement(element_index, rect);
    for (u32 element_index=0; element_index < g_ui.element_count; )
        element_index = CalculateTransforms(element_index, MAT3_IDENTITY);

    HandleInput();
}

void DrawUI() {
    for (u32 element_index = 0; element_index < g_ui.element_count; )
        element_index = RenderElement(element_index);
}

static Mesh* CreateElementQuad(Allocator* allocator)
{
    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(builder, {0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f});
    AddVertex(builder, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f});
    AddVertex(builder, {1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f});
    AddVertex(builder, {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 1.0f});
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);
    auto mesh = CreateMesh(allocator, builder, GetName("element"));
    PopScratch();
    return mesh;
}

void InitUI() {
    g_ui = {};
    g_ui.allocator = CreateArenaAllocator(sizeof(FatElement) * MAX_ELEMENTS, "UI");
    g_ui.camera = CreateCamera(ALLOCATOR_DEFAULT);
    g_ui.element_quad = CreateElementQuad(ALLOCATOR_DEFAULT);
    g_ui.element_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);
    g_ui.input = CreateInputSet(ALLOCATOR_DEFAULT);
    g_ui.text_mesh_allocator = CreatePoolAllocator(sizeof(CachedTextMesh), MAX_TEXT_MESHES);

    EnableButton(g_ui.input, MOUSE_LEFT);
    SetTexture(g_ui.element_material, TEXTURE_WHITE);
}

void ShutdownUI() {
    Destroy(g_ui.allocator);
    g_ui = {};
}

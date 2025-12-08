//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

constexpr int MAX_ELEMENTS = 4096;
constexpr int MAX_ELEMENT_STACK = 128;
constexpr int MAX_TEXT_MESHES = 4096;

extern void UpdateInputState(InputSet* input_set);

inline bool IsAuto(float v) { return v >= F32_AUTO; }
inline bool IsFixed(float v) { return !IsAuto(v); }

enum ElementType : u8 {
    ELEMENT_TYPE_UNKNOWN = 0,
    ELEMENT_TYPE_NONE,
    ELEMENT_TYPE_CANVAS,
    ELEMENT_TYPE_COLUMN,
    ELEMENT_TYPE_COLUMN_EXPANDED,
    ELEMENT_TYPE_COLUMN_SPACER,
    ELEMENT_TYPE_CONTAINER,
    ELEMENT_TYPE_IMAGE,
    ELEMENT_TYPE_LABEL,
    ELEMENT_TYPE_ROW,
    ELEMENT_TYPE_ROW_SPACER,
    ELEMENT_TYPE_ROW_EXPANDED,
    ELEMENT_TYPE_SCENE,
    ELEMENT_TYPE_TRANSFORM,
    ELEMENT_TYPE_COUNT
};

struct AlignInfo {
    bool has_x;
    float x;
    bool has_y;
    float y;
};

constexpr AlignInfo g_align_info[] = {
    { false, 0.0f, false, 0.0f },       // ALIGN_NONE
    { false, 0.0f, true,  0.0f },       // ALIGN_TOP
    { true,  0.0f, false, 0.0f },       // ALIGN_LEFT
    { false, 0.0f, true,  1.0f },       // ALIGN_BOTTOM
    { true,  1.0f, false, 0.0f },       // ALIGN_RIGHT
    { true,  0.0f, true,  0.0f },       // ALIGN_TOP_LEFT
    { true,  1.0f, true,  0.0f },       // ALIGN_TOP_RIGHT
    { true,  0.5f, true,  0.0f },       // ALIGN_TOP_CENTER
    { true,  0.0f, true,  0.5f },       // ALIGN_CENTER_LEFT
    { true,  1.0f, true,  0.5f },       // ALIGN_CENTER_RIGHT
    { true,  0.5f, true,  0.5f },       // ALIGN_CENTER
    { true,  0.0f, true,  1.0f },       // ALIGN_BOTTOM_LEFT
    { true,  1.0f, true,  1.0f },       // ALIGN_BOTTOM_RIGHT
    { true,  0.5f, true,  1.0f },       // ALIGN_BOTTOM_CENTER
};

static_assert(sizeof(g_align_info) / sizeof(AlignInfo) == ALIGN_COUNT);

struct CachedTextMesh {
    TextMesh* text_mesh;
    u64 hash;
    int last_frame;
};

struct ElementState {
    ElementFlags flags;
    u64 hash;
};

struct Element {
    ElementType type;
    u16 index;
    u16 next_sibling_index;
    u16 child_count;
    noz::Rect rect;
    Vec2 measured_size;
    Mat3 local_to_world;
    Mat3 world_to_local;
};

struct CanvasElement : Element {
    CanvasStyle style;
};

struct ColumnElement : Element {
    ColumnStyle style;
};

struct ContainerElement : Element {
    ContainerStyle style;
};

struct ExpandedElement : Element {
    ExpandedStyle style;
};

struct LabelElement : Element {
    LabelStyle style;
    CachedTextMesh* cached_mesh = nullptr;
};

struct ImageElement : Element {
    ImageStyle style;
    Mesh* mesh = nullptr;
    AnimatedMesh* animated_mesh = nullptr;
    float animated_time = 0.0f;
};

struct RowElement : Element {
    RowStyle style;
    float flex_total;
};

struct SceneElement : Element {
    SceneStyle style;
    void (*draw_scene)(void*);
};

struct SpacerElement : Element {
    float size;
};

struct TransformElement : Element {
    TransformStyle style;
};

union FatElement {
    Element element;
    CanvasElement canvas;
    ColumnElement column;
    ContainerElement container;
    ExpandedElement expanded;
    LabelElement label;
    ImageElement image;
    RowElement row;
    SpacerElement spacer;
};

struct UI {
    Allocator* allocator;
    Camera* camera;
    Camera* cursor_camera;
    Element* elements[MAX_ELEMENTS];
    Element* element_stack[MAX_ELEMENTS];
    ElementState element_states[MAX_ELEMENTS];
    u16 element_count;
    u16 element_stack_count;
    Mesh* element_quad;
    Mesh* image_mesh;
    Vec2 ortho_size;
    Vec2Int ref_size;
    Material* element_material;
    InputSet* input;
    PoolAllocator* text_mesh_allocator;
    float depth;
    u64 hash;
};

static UI g_ui = {};

static Element* CreateElement(ElementType type) {
    g_ui.hash = Hash(g_ui.hash, type);

    Element* element = static_cast<Element*>(Alloc(g_ui.allocator, sizeof(FatElement)));
    element->type = type;
    element->index = g_ui.element_count;
    element->next_sibling_index = element->index + 1;
    g_ui.elements[g_ui.element_count++] = element;

    ElementState& state = g_ui.element_states[element->index];
    if (state.hash != g_ui.hash) {
        state.flags = 0;
        state.hash = g_ui.hash;
    }

    if (g_ui.element_stack_count > 0) {
        g_ui.element_stack[g_ui.element_stack_count-1]->child_count++;
    }

    return element;
}

static Element* GetCurrentElement() {
    if (g_ui.element_stack_count <= 0)
        return nullptr;

    Element* e = g_ui.element_stack[g_ui.element_stack_count - 1];
    assert(e);
    return e;
}

Vec2 ScreenToElement(const Vec2& screen) {
    Element* e = GetCurrentElement();
    if (!e) return VEC2_ZERO;
    return TransformPoint(e->world_to_local, ScreenToWorld(g_ui.camera, screen));
}

u64 GetElementId() {
    Element* e = GetCurrentElement();
    if (!e) return 0;
    return g_ui.element_states[e->index].hash;
}

bool CheckElementFlags(ElementFlags flags) {
    if (g_ui.element_stack_count <= 0)
        return false;

    Element* e = g_ui.element_stack[g_ui.element_stack_count - 1];
    assert(e);

    return (g_ui.element_states[e->index].flags & flags) == flags;
}

Vec2 ScreenToUI(const Vec2& screen_pos) {
    return screen_pos / ToVec2(GetScreenSize()) * g_ui.ortho_size;
}

static void PushElement(Element* element) {
    if (g_ui.element_stack_count >= MAX_ELEMENT_STACK)
        return;

    g_ui.element_stack[g_ui.element_stack_count++] = element;
}

static void PopElement() {
    if (g_ui.element_stack_count == 0)
        return;

    GetCurrentElement()->next_sibling_index = g_ui.element_count;
    g_ui.element_stack_count--;
}

static void EndElement(ElementType expected_type) {
    assert(GetCurrentElement()->type == expected_type);
    PopElement();
}

void BeginBorder(const BorderStyle& style) {
    ContainerElement* e = static_cast<ContainerElement*>(CreateElement(ELEMENT_TYPE_CONTAINER));
    e->style = { .border = style };
    PushElement(e);
}

void EndBorder() {
    EndElement(ELEMENT_TYPE_CONTAINER);
}

void BeginColumn(const ColumnStyle& style) {
    ColumnElement* column = static_cast<ColumnElement*>(CreateElement(ELEMENT_TYPE_COLUMN));
    column->style = style;
    PushElement(column);
}

void EndColumn() {
    EndElement(ELEMENT_TYPE_COLUMN);
}

// @canvas
void BeginCanvas(const CanvasStyle& style) {
    CanvasElement* canvas = static_cast<CanvasElement*>(CreateElement(ELEMENT_TYPE_CANVAS));
    canvas->style = style;
    PushElement(canvas);
}

void EndCanvas() {
    EndElement(ELEMENT_TYPE_CANVAS);
}

// @center
void BeginCenter() {
    ContainerElement* e = static_cast<ContainerElement*>(CreateElement(ELEMENT_TYPE_CONTAINER));
    e->style = {.align = ALIGN_CENTER };
    PushElement(e);
}

void EndCenter() {
    EndElement(ELEMENT_TYPE_CONTAINER);
}

// @container
void BeginContainer(const ContainerStyle& style) {
    ContainerElement* e = static_cast<ContainerElement*>(CreateElement(ELEMENT_TYPE_CONTAINER));
    e->style = style;
    PushElement(e);
}

void EndContainer() {
    EndElement(ELEMENT_TYPE_CONTAINER);
}

void Container(const ContainerStyle& style) {
    BeginContainer(style);
    EndContainer();
}

// @row
void BeginRow(const RowStyle& style) {
    RowElement* row = static_cast<RowElement*>(CreateElement(ELEMENT_TYPE_ROW));
    row->style = style;
    PushElement(row);
}

void EndRow() {
    EndElement(ELEMENT_TYPE_ROW);
}

// @expanded
void BeginExpanded(const ExpandedStyle& style) {
    Element* parent = GetCurrentElement();
    assert(parent);
    assert(parent->type == ELEMENT_TYPE_ROW || parent->type == ELEMENT_TYPE_COLUMN);
    ElementType expanded_type = parent->type == ELEMENT_TYPE_ROW
        ? ELEMENT_TYPE_ROW_EXPANDED
        : ELEMENT_TYPE_COLUMN_EXPANDED;

    ExpandedElement* e = static_cast<ExpandedElement*>(CreateElement(expanded_type));
    e->style = style;
    PushElement(e);
}

void EndExpanded() {
    assert(
        GetCurrentElement()->type == ELEMENT_TYPE_ROW_EXPANDED ||
        GetCurrentElement()->type == ELEMENT_TYPE_COLUMN_EXPANDED);
    EndElement(GetCurrentElement()->type);
}

void Expanded(const ExpandedStyle& style) {
    BeginExpanded(style);
    EndExpanded();
}

// @spacer
void Spacer(float size) {
    Element* parent = GetCurrentElement();
    assert(parent);
    assert(parent->type == ELEMENT_TYPE_ROW || parent->type == ELEMENT_TYPE_COLUMN);
    ElementType spacer_type = parent->type == ELEMENT_TYPE_ROW
        ? ELEMENT_TYPE_ROW_SPACER
        : ELEMENT_TYPE_COLUMN_SPACER;
    SpacerElement* e = static_cast<SpacerElement*>(CreateElement(spacer_type));
    e->size = size;
}

static u64 GetMeshHash(const TextRequest& request) {
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
        if (TextMesh* tm = CreateTextMesh(ALLOCATOR_DEFAULT, r)) {
            args.result = static_cast<CachedTextMesh*>(Alloc(g_ui.text_mesh_allocator, sizeof(CachedTextMesh)));
            args.result->hash = args.hash;
            args.result->text_mesh = tm;
            args.result->last_frame = 0;
        }
    }

    return args.result;
}

void Label(const char* text, const LabelStyle& style) {
    LabelElement* label = static_cast<LabelElement*>(CreateElement(ELEMENT_TYPE_LABEL));
    label->style = style;
    label->cached_mesh = GetOrCreateTextMesh(text, style);
}

void Image(Mesh* mesh, const ImageStyle& style) {
    ImageElement* image = static_cast<ImageElement*>(CreateElement(ELEMENT_TYPE_IMAGE));
    image->mesh = mesh;
    image->style = style;
    if (!image->style.material)
        image->style.material = g_ui.element_material;
}

void Image(AnimatedMesh* mesh, float time, const ImageStyle& style) {
    ImageElement* image = static_cast<ImageElement*>(CreateElement(ELEMENT_TYPE_IMAGE));
    image->animated_mesh = mesh;
    image->animated_time = time;
    image->style = style;
    if (!image->style.material)
        image->style.material = g_ui.element_material;
}

void Rectangle(const RectangleStyle& style) {
    ContainerElement* e = static_cast<ContainerElement*>(CreateElement(ELEMENT_TYPE_CONTAINER));
    e->style = {};
    e->style.width = style.width;
    e->style.height = style.height;
    e->style.color = style.color;
}

void Scene(const SceneStyle& style, void (*draw_scene)(void*)) {
    SceneElement* e = static_cast<SceneElement*>(CreateElement(ELEMENT_TYPE_SCENE));
    e->style = style;
    e->draw_scene = draw_scene;
}

void BeginTransformed(const TransformStyle& style) {
    TransformElement* transform = static_cast<TransformElement*>(CreateElement(ELEMENT_TYPE_TRANSFORM));
    transform->style = style;
    PushElement(transform);
}

void EndTransformed() {
    EndElement(ELEMENT_TYPE_TRANSFORM);
}

static int MeasureElement(int element_index, const Vec2& available_size) {
    Element* e = g_ui.elements[element_index++];
    assert(e);

    if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        Vec2 content_size = VEC2_ZERO;
        bool is_auto_width = IsAuto(container->style.width);
        bool is_auto_height = IsAuto(container->style.height);
        if (is_auto_width)
            content_size.x = available_size.x -
                container->style.margin.left -
                container->style.margin.right;
        else
            content_size.x = container->style.width;

        if (is_auto_height)
            content_size.y = available_size.y -
                container->style.margin.top -
                container->style.margin.bottom;
        else
            content_size.y = container->style.height;

        content_size.x -=
            container->style.padding.left +
            container->style.padding.right +
            container->style.border.width * 2.0f;

        content_size.y -=
            container->style.padding.top +
            container->style.padding.bottom +
            container->style.border.width * 2.0f;

        Vec2 max_content_size = VEC2_ZERO;
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = MeasureElement(element_index, content_size);
            max_content_size = Max(max_content_size, child->measured_size);
        }

        const AlignInfo& align = g_align_info[container->style.align];

        if (!is_auto_width)
            e->measured_size.x = container->style.width;
        else if (e->child_count == 0 || !align.has_x)
            e->measured_size.x = available_size.x;
        else
            e->measured_size.x = max_content_size.x +
                container->style.padding.left +
                container->style.padding.right +
                container->style.border.width * 2.0f;

        if (!is_auto_height)
            e->measured_size.y = container->style.height;
        else if (e->child_count == 0 || !align.has_y)
            e->measured_size.y = available_size.y;
        else
            e->measured_size.y = max_content_size.y +
                container->style.padding.top +
                container->style.padding.bottom +
                container->style.border.width * 2.0f;

    } else if (e->type == ELEMENT_TYPE_ROW) {
        Vec2 max_content_size = VEC2_ZERO;
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = MeasureElement(element_index, available_size);
            max_content_size.x += child->measured_size.x;
            max_content_size.y = Max(max_content_size.y, child->measured_size.y);

            if (child->type == ELEMENT_TYPE_ROW_EXPANDED) {
                RowElement* row = static_cast<RowElement*>(e);
                ExpandedElement* expanded = static_cast<ExpandedElement*>(child);
                row->flex_total += expanded->style.flex;
            }
        }
        e->measured_size = max_content_size;
    } else if (e->type == ELEMENT_TYPE_COLUMN) {
        Vec2 max_content_size = VEC2_ZERO;
        int child_element_index = element_index;
        float flex_total = 0.0f;
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = MeasureElement(element_index, available_size);
            max_content_size.x = Max(max_content_size.x, child->measured_size.x);
            max_content_size.y += child->measured_size.y;
            if (child->type == ELEMENT_TYPE_COLUMN_EXPANDED)
                flex_total += static_cast<ExpandedElement*>(child)->style.flex;
        }

        e->measured_size = max_content_size;
        if (flex_total >= F32_EPSILON) {
            e->measured_size.y = Max(max_content_size.y, available_size.y);
            float flex_available = Max(0.0f, available_size.y - max_content_size.y);
            for (u16 i = 0; i < e->child_count; i++) {
                Element* child = g_ui.elements[child_element_index];
                if (child->type == ELEMENT_TYPE_COLUMN_EXPANDED) {
                    ExpandedElement* expanded = static_cast<ExpandedElement*>(child);
                    child->measured_size.y = expanded->style.flex / flex_total * flex_available;
                    MeasureElement(child_element_index, Vec2{available_size.x, child->measured_size.y});
                }
                child_element_index = child->next_sibling_index;
            }
        }
    } else if (e->type == ELEMENT_TYPE_LABEL) {
        LabelElement* l = static_cast<LabelElement*>(e);
        if (l->cached_mesh && l->cached_mesh->text_mesh) {
            Vec2 text_size = GetSize(l->cached_mesh->text_mesh);
            e->measured_size = text_size;
        }

        const AlignInfo& align = g_align_info[l->style.align];
        if (align.has_x) {
            e->measured_size.x = Max(e->measured_size.x, available_size.x);
        }
        if (align.has_y) {
            e->measured_size.y = Max(e->measured_size.y, available_size.y);
        }

        assert(e->child_count == 0);
    } else if (e->type == ELEMENT_TYPE_IMAGE) {
        ImageElement* i = static_cast<ImageElement*>(e);
        if (i->animated_mesh) {
            Vec2 mesh_size = GetSize(i->animated_mesh);
            e->measured_size = mesh_size * i->style.scale;
        } else if (i->mesh) {
            Vec2 mesh_size = GetSize(i->mesh);
            e->measured_size = mesh_size * i->style.scale;
        }

        const AlignInfo& align = g_align_info[i->style.align];
        if (align.has_x)
            e->measured_size.x = Max(e->measured_size.x, available_size.x);
        if (align.has_y )
            e->measured_size.y = Max(e->measured_size.y, available_size.y);

        if (i->style.stretch == IMAGE_STRETCH_FILL) {
            e->measured_size = available_size;
        } else if (i->style.stretch == IMAGE_STRETCH_UNIFORM) {
            float aspect_ratio = e->measured_size.x / e->measured_size.y;
            if (available_size.x / available_size.y > aspect_ratio) {
                e->measured_size.y = available_size.y;
                if (align.has_x)
                    e->measured_size.x = available_size.x;
                else
                    e->measured_size.x = e->measured_size.y * aspect_ratio;
            } else {
                e->measured_size.x = available_size.x;
                if (align.has_y)
                    e->measured_size.y = available_size.x;
                else
                    e->measured_size.y = e->measured_size.x / aspect_ratio;
            }
        }

        assert(e->child_count == 0);
    } else if (e->type == ELEMENT_TYPE_CANVAS) {
        CanvasElement* canvas = static_cast<CanvasElement*>(e);
        if (canvas->style.type == CANVAS_TYPE_SCREEN) {
            e->measured_size = available_size;
            for (u16 i = 0; i < e->child_count; i++)
                element_index = MeasureElement(element_index, available_size);

        // } else if (canvas->style.type == CANVAS_TYPE_WORLD) {
            // Vec2 screen_size = Abs(WorldToScreen(canvas->style.world_camera, canvas->style.world_size) - WorldToScreen(canvas->style.world_camera, VEC2_ZERO));
            // Vec2 ui_size = ScreenToWorld(g_ui.camera, screen_size) - ScreenToWorld(g_ui.camera, VEC2_ZERO);
            // e->measured_size = ui_size;
            //
            // Vec2 screen_pos = WorldToScreen(c->style.world_camera, c->style.world_position);
            // Vec2 screen_size = Abs(WorldToScreen(c->style.world_camera, c->style.world_size) - WorldToScreen(c->style.world_camera, VEC2_ZERO));
            // screen_pos = screen_pos - screen_size * 0.5f;
            //
            // Vec2 ui_pos = ScreenToWorld(g_ui.camera, screen_pos);
            // Vec2 ui_size = ScreenToWorld(g_ui.camera, screen_size) - ScreenToWorld(g_ui.camera, VEC2_ZERO);
            // e->rect.x = ui_pos.x;
            // e->rect.y = ui_pos.y;
            // contraints = ui_size;

        } else {
            assert(false && "Unknown canvas type");
        }
    } else if (e->type == ELEMENT_TYPE_SCENE) {
        e->measured_size = available_size;
    } else if (e->type == ELEMENT_TYPE_ROW_SPACER) {
        e->measured_size.x = static_cast<SpacerElement*>(e)->size;
        e->measured_size.y = 0;
    } else if (e->type == ELEMENT_TYPE_COLUMN_SPACER) {
        e->measured_size.x = 0;
        e->measured_size.y = static_cast<SpacerElement*>(e)->size;
    } else if (e->type == ELEMENT_TYPE_ROW_EXPANDED) {
        Vec2 max_content_size = {};
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = MeasureElement(element_index, available_size);
            max_content_size.y = Max(max_content_size.y, child->measured_size.y);
        }
        e->measured_size.y = max_content_size.y;
    } else if (e->type == ELEMENT_TYPE_COLUMN_EXPANDED) {
        Vec2 max_content_size = {};
        Vec2 content_size = Vec2{available_size.x, e->measured_size.y};
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = MeasureElement(element_index, content_size);
            max_content_size.x = Max(max_content_size.y, child->measured_size.x);
        }

        e->measured_size.x = max_content_size.x;
    } else {
        assert(false && "Unhandled element type in MeasureElements");
    }

    return element_index;
}

static int LayoutElement(int element_index, const Vec2& size, Element* parent) {
    Element* e = g_ui.elements[element_index++];
    assert(e);

    e->rect = noz::Rect{0, 0, e->measured_size.x, e->measured_size.y};

    if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        const AlignInfo& align_info = g_align_info[container->style.align];
        bool subtract_margin_x = false;
        bool subtract_margin_y = false;
        if (align_info.has_x) {
            float available_width = size.x - e->rect.width - container->style.margin.left - container->style.margin.right;
            e->rect.x = available_width * align_info.x;
        } else if (IsAuto(container->style.width)) {
            e->rect.width = size.x;
            subtract_margin_x = true;
        }
        if (align_info.has_y) {
            float available_height = size.y - e->rect.height - container->style.margin.top - container->style.margin.bottom;
            e->rect.y = available_height * align_info.y;
        } else if (IsAuto(container->style.height)) {
            e->rect.height = size.y;
            subtract_margin_y = true;
        }

        Vec2 child_offset = {
            container->style.padding.left + container->style.border.width,
            container->style.padding.top + container->style.border.width
        };

        Vec2 content_size = GetSize(e->rect);
        content_size.x -=
            container->style.padding.left +
            container->style.padding.right +
            container->style.border.width * 2.0f;
        content_size.y -=
            container->style.padding.top +
            container->style.padding.bottom +
            container->style.border.width * 2.0f;

        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = LayoutElement(element_index, content_size, e);
            child->rect.x += child_offset.x;
            child->rect.y += child_offset.y;
        }

        e->rect.x += container->style.margin.left;
        e->rect.y += container->style.margin.top;

        if (subtract_margin_x)
            e->rect.width -= container->style.margin.left + container->style.margin.right;

        if (subtract_margin_y)
            e->rect.height -= container->style.margin.top + container->style.margin.bottom;

    } else if (e->type == ELEMENT_TYPE_COLUMN) {
        Vec2 content_size = GetSize(e->rect);
        float child_offset = 0.0f;
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = LayoutElement(element_index, content_size, e);
            child->rect.y += child_offset;
            child_offset += child->measured_size.y;
        }
    } else if (e->type == ELEMENT_TYPE_ROW) {
        Vec2 content_size = GetSize(e->rect);
        Vec2 child_offset = {};
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = LayoutElement(element_index, content_size, e);
            child->rect.x = child_offset.x;
            child_offset.x = child->rect.x + child->rect.width;
        }
    } else if (e->type == ELEMENT_TYPE_LABEL) {
    } else if (e->type == ELEMENT_TYPE_IMAGE) {
    } else if (e->type == ELEMENT_TYPE_ROW_EXPANDED) {
        assert(parent && parent->type == ELEMENT_TYPE_ROW);
        RowElement* row = static_cast<RowElement*>(parent);
        ExpandedElement* expanded = static_cast<ExpandedElement*>(e);
        float flex_available = size.x - row->measured_size.x;
        if (flex_available > F32_EPSILON && row->flex_total)
            e->rect.width = flex_available * (expanded->style.flex / row->flex_total);

        Vec2 content_size = GetSize(e->rect);
        for (u16 i = 0; i < e->child_count; i++)
            element_index = LayoutElement(element_index, content_size, e);
    } else if (e->type == ELEMENT_TYPE_COLUMN_EXPANDED) {
        assert(parent && parent->type == ELEMENT_TYPE_COLUMN);
        // ColumnElement* column = static_cast<ColumnElement*>(parent);
        // ExpandedElement* expanded = static_cast<ExpandedElement*>(e);
        // if (column->flex_available > F32_EPSILON && column->flex_total)
        //     e->rect.height = column->flex_available * (expanded->style.flex / column->flex_total);

        Vec2 content_size = GetSize(e->rect);
        for (u16 i = 0; i < e->child_count; i++)
            element_index = LayoutElement(element_index, content_size, e);
    } else if (e->type == ELEMENT_TYPE_SCENE) {
        e->type = ELEMENT_TYPE_SCENE;
    } else if (e->type == ELEMENT_TYPE_CANVAS) {
    } else if (e->type == ELEMENT_TYPE_COLUMN_SPACER) {
    } else if (e->type == ELEMENT_TYPE_ROW_SPACER) {
    } else {
        assert(false && "Unhandled element type in LayoutElements");
    }

    return element_index;
}

static u32 CalculateTransforms(u32 element_index, const Mat3& parent_transform) {
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

void RenderCanvas(Element* e){
    (void)e;
    UpdateCamera(g_ui.camera);
    BindCamera(g_ui.camera);
}

static void RenderBackground(const noz::Rect& rect, const Mat3& transform, const Color& color, const Vec2Int& color_offset) {
    if (color.a <= 0.0f)
        return;

    BindTransform(transform * Scale(Vec2{rect.width, rect.height}));
    BindMaterial(g_ui.element_material);
    BindColor(color,color_offset);
    DrawMesh(g_ui.element_quad);
}

static int RenderElement(int element_index) {
    Element* e = g_ui.elements[element_index++];
    const Mat3& transform = e->local_to_world;

    if (e->type == ELEMENT_TYPE_CANVAS)
        RenderCanvas(e);

    if (e->type == ELEMENT_TYPE_LABEL) {
        LabelElement* l = static_cast<LabelElement*>(e);
        Mesh* mesh = l->cached_mesh ? GetMesh(l->cached_mesh->text_mesh) : nullptr;
        if (mesh) {
            const AlignInfo& align = g_align_info[l->style.align];
            Vec2 text_size = GetSize(l->cached_mesh->text_mesh);
            Vec2 text_offset = VEC2_ZERO;
            if (align.has_x)
                text_offset.x = (e->rect.width - text_size.x) * align.x;
            if (align.has_y)
                text_offset.y = (e->rect.height - text_size.y) * align.y;

            BindTransform(e->local_to_world * Translate(text_offset));
            BindColor(l->style.color);
            BindMaterial(l->style.material ? l->style.material : GetMaterial(l->cached_mesh->text_mesh));
            DrawMesh(mesh);
        }
    } else if (e->type == ELEMENT_TYPE_IMAGE) {
        ImageElement* image = static_cast<ImageElement*>(e);
        BindMaterial(image->style.material);
        Bounds2 mesh_bounds = image->animated_mesh ? GetBounds(image->animated_mesh) : GetBounds(image->mesh);
        Vec2 mesh_size = GetSize(mesh_bounds);

        BindColor(image->style.color, image->style.color_offset);

        Mat3 image_transform;
        if (image->style.stretch == IMAGE_STRETCH_UNIFORM) {
            float scale_x = e->rect.width / mesh_size.x;
            float scale_y = e->rect.height / mesh_size.y;
            float uniform_scale = Min(scale_x, scale_y) * image->style.scale;

            const AlignInfo& align = g_align_info[image->style.align];
            Vec2 image_offset = VEC2_ZERO;
            if (align.has_x)
                image_offset.x = (e->rect.width - mesh_size.x * uniform_scale) * align.x;
            if (align.has_y)
                image_offset.y = (e->rect.height - mesh_size.y * uniform_scale) * align.y;

            image_transform = transform *
                Translate(Vec2{-mesh_bounds.min.x, mesh_bounds.max.y} * uniform_scale + image_offset) *
                Scale(Vec2{uniform_scale, -uniform_scale});
        } else if (image->style.stretch == IMAGE_STRETCH_FILL) {
        //     Vec2 mesh_scale = Vec2{
        //         e->rect.width / mesh_size.x,
        //         e->rect.height / mesh_size.y
        //     };
        //     image_transform = transform *
        //         Translate(-mesh_bounds.min * mesh_scale + image_offset) *
        //         Scale(mesh_scale) *
        //         Scale(Vec2{1, -1} * image->style.scale);
        } else {
            // float uniform_scale = image->style.scale;
            // image_transform = transform *
            //     Translate(Vec2{-mesh_bounds.min.x, mesh_bounds.max.y} * uniform_scale  + image_offset) *
            //     Scale(Vec2{uniform_scale, -uniform_scale});
        }

        if (image->animated_mesh)
            DrawMesh(image->animated_mesh, image_transform, image->animated_time);
        else
            DrawMesh(image->mesh, image_transform);
    } else if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        RenderBackground(e->rect, transform, container->style.color, container->style.color_offset);

        if (container->style.border.width > 0.0f && container->style.border.color.a > 0) {
            float border_width = container->style.border.width;
            BindColor(container->style.border.color);
            BindMaterial(g_ui.element_material);
            BindTransform(transform * Scale(Vec2{e->rect.width, border_width}));
            DrawMesh(g_ui.element_quad);
            BindTransform(transform * Translate(Vec2{0, e->rect.height - border_width}) * Scale(Vec2{e->rect.width, border_width}));
            DrawMesh(g_ui.element_quad);
            BindTransform(transform * Translate(Vec2{0, border_width}) * Scale(Vec2{border_width, e->rect.height - border_width * 2}));
            DrawMesh(g_ui.element_quad);
            BindTransform(transform * Translate(Vec2{e->rect.width - border_width, border_width}) * Scale(Vec2{border_width, e->rect.height - border_width * 2}));
            DrawMesh(g_ui.element_quad);
        }

    } else if (e->type == ELEMENT_TYPE_CANVAS) {
        CanvasElement* canvas = static_cast<CanvasElement*>(e);
        RenderBackground(e->rect, transform, canvas->style.color, VEC2INT_ZERO);
    } else if (e->type == ELEMENT_TYPE_SCENE) {
        SceneElement* scene_element = static_cast<SceneElement*>(e);
        if (scene_element->style.camera && scene_element->draw_scene) {
            // Calculate the screen rect for the scene element
            // Transform the element corners from UI space to screen space
            Vec2 ui_top_left = TransformPoint(transform, VEC2_ZERO);
            Vec2 ui_bottom_right = TransformPoint(transform, Vec2{e->rect.width, e->rect.height});

            Vec2 screen_top_left = WorldToScreen(g_ui.camera, ui_top_left);
            Vec2 screen_bottom_right = WorldToScreen(g_ui.camera, ui_bottom_right);

            // Create viewport rect in screen pixels
            noz::Rect viewport_rect = {
                screen_top_left.x,
                screen_top_left.y,
                screen_bottom_right.x - screen_top_left.x,
                screen_bottom_right.y - screen_top_left.y
            };

            // Set the scene camera's viewport and render
            SetViewport(scene_element->style.camera, viewport_rect);
            UpdateCamera(scene_element->style.camera);
            BindCamera(scene_element->style.camera);
            scene_element->draw_scene(scene_element->style.user_data);

            // Restore the UI camera
            UpdateCamera(g_ui.camera);
            BindCamera(g_ui.camera);
            BindDepth(g_ui.depth, 0);
        }
    }

    for (u32 i = 0; i < e->child_count; i++)
        element_index = RenderElement(element_index);

    return element_index;
}

void BeginUI(u32 ref_width, u32 ref_height) {
    g_ui.ref_size = { static_cast<i32>(ref_width), static_cast<i32>(ref_height) };
    g_ui.element_stack_count = 0;
    g_ui.element_count = 0;
    g_ui.hash = 0;

    Clear(g_ui.allocator);

    Vec2Int screen_size = GetScreenSize();

    f32 rw = static_cast<f32>(g_ui.ref_size.x);
    f32 rh = static_cast<f32>(g_ui.ref_size.y);
    f32 sw = static_cast<f32>(screen_size.x);
    f32 sh = static_cast<f32>(screen_size.y);
    f32 sw_rw = sw / rw;
    f32 sh_rh = sh / rh;

    if (Abs(sw_rw - 1.0f) < Abs(sh_rh - 1.0f)) {
        g_ui.ortho_size.x = rw;
        g_ui.ortho_size.y = rw * sh / sw;
    } else {
        g_ui.ortho_size.y = rh;
        g_ui.ortho_size.x = rh * sw / sh;
    }

    SetExtents(g_ui.camera, 0, g_ui.ortho_size.x, 0, g_ui.ortho_size.y, false);

    UpdateInputState(g_ui.input);
}

static void HandleInput() {
    Vec2 mouse = ScreenToWorld(g_ui.camera, GetMousePosition());
    for (u32 i=g_ui.element_count; i>0; i--) {
        Element* e = g_ui.elements[i-1];
        ElementState& prev_state = g_ui.element_states[i-1];
        Vec2 local_mouse = TransformPoint(e->world_to_local, mouse);
        bool mouse_over = Contains(Bounds2{0,0,e->rect.width, e->rect.height}, local_mouse);

        if (mouse_over)
            prev_state.flags = prev_state.flags | ELEMENT_FLAG_HOVERED;
        else
            prev_state.flags = prev_state.flags & ~ELEMENT_FLAG_HOVERED;

        if (mouse_over && WasButtonPressed(g_ui.input, MOUSE_LEFT)) {
            prev_state.flags = prev_state.flags | ELEMENT_FLAG_PRESSED;
        } else {
            prev_state.flags = prev_state.flags & ~ELEMENT_FLAG_PRESSED;
        }

        if ((prev_state.flags & ELEMENT_FLAG_PRESSED) && e->type != ELEMENT_TYPE_CANVAS) {
            ConsumeButton(MOUSE_LEFT);
        }

        if (mouse_over && IsButtonDown(g_ui.input, MOUSE_LEFT)) {
            prev_state.flags = prev_state.flags | ELEMENT_FLAG_DOWN;
        } else {
            prev_state.flags = prev_state.flags & ~ELEMENT_FLAG_DOWN;
        }
    }
}

void EndUI() {
    for (u32 element_index=0; element_index < g_ui.element_count; )
        element_index = MeasureElement(element_index, g_ui.ortho_size);
    for (u32 element_index=0; element_index < g_ui.element_count; )
        element_index = LayoutElement(element_index, g_ui.ortho_size, nullptr);
    for (u32 element_index=0; element_index < g_ui.element_count; )
        element_index = CalculateTransforms(element_index, MAT3_IDENTITY);

    HandleInput();
}

void DrawUI() {
    BindDepth(g_ui.depth, 0);
    for (u32 element_index = 0; element_index < g_ui.element_count; )
        element_index = RenderElement(element_index);

    // cursor
    BindDepth(g_ui.depth, 0);
    if (GetApplicationTraits()->draw_cursor) {
        Vec2 screen_size = ToVec2(GetScreenSize());
        Vec2 cursor_pos = GetMousePosition();
        cursor_pos.y = screen_size.y - cursor_pos.y;
        SetExtents(g_ui.cursor_camera, 0, screen_size.x, screen_size.y, 0);
        BindCamera(g_ui.cursor_camera);
        GetApplicationTraits()->draw_cursor(Translate(cursor_pos));
    }
}


static Mesh* CreateElementQuad(Allocator* allocator) {
    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(builder, Vec2{0.0f, 0.0f}, ColorUV(0,0));
    AddVertex(builder, Vec2{1.0f, 0.0f}, ColorUV(0,0));
    AddVertex(builder, Vec2{1.0f, 1.0f}, ColorUV(0,0));
    AddVertex(builder, Vec2{0.0f, 1.0f}, ColorUV(0,0));
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);
    auto mesh = CreateMesh(allocator, builder, GetName("element"));
    PopScratch();
    return mesh;
}

static Mesh* CreateImageQuad(Allocator* allocator) {
    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(builder, Vec2{ -0.5f, -0.5f }, { 0.0f, 1.0f });
    AddVertex(builder, Vec2{  0.5f, -0.5f }, { 1.0f, 1.0f });
    AddVertex(builder, Vec2{  0.5f,  0.5f }, { 1.0f, 0.0f });
    AddVertex(builder, Vec2{ -0.5f,  0.5f }, { 0.0f, 0.0f });
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);
    auto mesh = CreateMesh(allocator, builder, GetName("image"));
    PopScratch();
    return mesh;
}

void SetUIPaletteTexture(Texture* texture) {
    if (!texture) return;
    SetTexture(g_ui.element_material, texture);
}

void InitUI(const ApplicationTraits* traits) {
    g_ui = {};
    g_ui.allocator = CreateArenaAllocator(sizeof(FatElement) * MAX_ELEMENTS, "UI");
    g_ui.camera = CreateCamera(ALLOCATOR_DEFAULT);
    g_ui.element_quad = CreateElementQuad(ALLOCATOR_DEFAULT);
    g_ui.image_mesh = CreateImageQuad(ALLOCATOR_DEFAULT);
    g_ui.element_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);
    g_ui.input = CreateInputSet(ALLOCATOR_DEFAULT);
    g_ui.text_mesh_allocator = CreatePoolAllocator(sizeof(CachedTextMesh), MAX_TEXT_MESHES);
    g_ui.depth = traits->ui_depth >= F32_MAX ? traits->renderer.max_depth - 0.01f : traits->ui_depth;

    g_ui.cursor_camera = CreateCamera(ALLOCATOR_DEFAULT);

    EnableButton(g_ui.input, MOUSE_LEFT);
    if (TEXTURE)
        SetUIPaletteTexture(TEXTURE[0]);
}

void ShutdownUI() {
    Destroy(g_ui.allocator);
    g_ui = {};
}

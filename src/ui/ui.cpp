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
    ELEMENT_TYPE_ALIGN,
    ELEMENT_TYPE_BORDER,
    ELEMENT_TYPE_CANVAS,
    ELEMENT_TYPE_CENTER,
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
    u32 child_count;
    Mat3 local_to_world;
    Mat3 world_to_local;
};

struct AlignElement : Element {
    AlignStyle style;
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
    void* user_data;
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

struct SizedBoxElement : Element {
    SizedBoxStyle style;
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
    AlignElement align;
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
    SizedBoxElement sized_box;
};

struct UI {
    Allocator* allocator;
    Camera* camera;
    Element* elements[MAX_ELEMENTS];
    Element* element_stack[MAX_ELEMENTS];
    u32 element_stack_count;
    Mesh* element_quad;
    Mesh* image_mesh;
    u32 element_count;
    Vec2 ortho_size;
    Vec2Int ref_size;
    Material* element_material;
    InputSet* input;
    PoolAllocator* text_mesh_allocator;
    ElementState prev_element_states[MAX_ELEMENTS];
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
    if (g_ui.element_stack_count == 0)
        return;
    g_ui.element_stack[g_ui.element_stack_count-1]->child_count++;
}

static void ExecuteChildren(Element* element, const std::function<void()>& children) {
    if (!children)
        return;

    PushElement(element);
    children();
    PopElement();
}

void Align(const AlignStyle& style, const std::function<void()>& children) {
    IncrementChildCount();
    AlignElement* e = static_cast<AlignElement*>(CreateElement(ELEMENT_TYPE_ALIGN));
    e->style = style;
    ExecuteChildren(e, children);
}

void Border(const BorderStyle& style, const std::function<void()>& children) {
    IncrementChildCount();
    BorderElement* border = static_cast<BorderElement*>(CreateElement(ELEMENT_TYPE_BORDER));
    border->style = style;
    ExecuteChildren(border, children);
}

void Canvas(const CanvasStyle& style, const std::function<void()>& children) {
    IncrementChildCount();
    CanvasElement* canvas = static_cast<CanvasElement*>(CreateElement(ELEMENT_TYPE_CANVAS));
    canvas->style = style;
    ExecuteChildren(canvas, children);
}

void Center(const std::function<void()>& children) {
    IncrementChildCount();
    Element* center = CreateElement(ELEMENT_TYPE_CENTER);
    ExecuteChildren(center, children);
}

void Row(const RowStyle& style, const std::function<void()>& children) {
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

void Container(const ContainerStyle& style, const std::function<void()>& children) {
    IncrementChildCount();
    ContainerElement* container = static_cast<ContainerElement*>(CreateElement(ELEMENT_TYPE_CONTAINER));
    container->style = style;
    ExecuteChildren(container, children);
}

void Expanded(const ExpandedStyle& style, const std::function<void()>& children) {
    IncrementChildCount();
    ExpandedElement* e = static_cast<ExpandedElement*>(CreateElement(ELEMENT_TYPE_EXPANDED));
    e->style = style;
    ExecuteChildren(e, children);
}

void GestureDetector(const GestureDetectorStyle& style, const std::function<void()>& children) {
    IncrementChildCount();
    GestureDetectorElement* gesture_detector = static_cast<GestureDetectorElement*>(CreateElement(ELEMENT_TYPE_GESTURE_DETECTOR));
    gesture_detector->style = style;
    ExecuteChildren(gesture_detector, children);
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
    image->mesh = g_ui.image_mesh;
    image->style = style;
}

void Image(Material* material, Mesh* mesh, const ImageStyle& style) {
    IncrementChildCount();
    ImageElement* image = static_cast<ImageElement*>(CreateElement(ELEMENT_TYPE_IMAGE));
    image->material = material;
    image->mesh = mesh;
    image->style = style;
}

void MouseRegion(const MouseRegionStyle& style, const std::function<void()>& children) {
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

void SizedBox(const SizedBoxStyle& style, const std::function<void()>& children) {
    IncrementChildCount();
    SizedBoxElement* e = static_cast<SizedBoxElement*>(CreateElement(ELEMENT_TYPE_SIZED_BOX));
    e->style = style;
    ExecuteChildren(e, children);
}

void Transformed(const TransformStyle& style, void (*children)()) {
    IncrementChildCount();
    TransformElement* transform = static_cast<TransformElement*>(CreateElement(ELEMENT_TYPE_TRANSFORM));
    transform->style = style;
    ExecuteChildren(transform, children);
}

static int LayoutElement(int element_index, const Vec2& constraints, Element* parent);

static int SkipElement(int element_index) {
    Element* e = g_ui.elements[element_index++];
    for (u32 i = 0; i < e->child_count; i++)
        element_index = SkipElement(element_index);
    return element_index;
}

static EdgeInsets GetMargin(Element* e) {
    if (e->type == ELEMENT_TYPE_CONTAINER) {
        return static_cast<ContainerElement*>(e)->style.margin;
    } else if (e->type == ELEMENT_TYPE_ALIGN) {
        return static_cast<AlignElement*>(e)->style.margin;
    }
    return {};
}

static void ApplyAlignment(Element* e, const Alignment& align, u32 element_stack_start, u32 element_stack_count) {
    EdgeInsets parent_margin = GetMargin(e);
    for (u32 i = 0; i < element_stack_count; i++) {
        Element* child = g_ui.element_stack[element_stack_start + i];
        EdgeInsets child_margin = GetMargin(child);
        // Calculate available space excluding parent's margins
        float available_width = e->rect.width - parent_margin.left - parent_margin.right;
        float available_height = e->rect.height - parent_margin.top - parent_margin.bottom;
        // Calculate final position including both parent and child margins
        child->rect.x = parent_margin.left + child_margin.left + (available_width - child->rect.width - child_margin.left - child_margin.right) * 0.5f * (align.x + 1.0f);
        child->rect.y = parent_margin.top + child_margin.top + (available_height - child->rect.height - child_margin.top - child_margin.bottom) * 0.5f * (align.y + 1.0f);
    }
}

static int LayoutChildren(int element_index, Element* parent, const Vec2& size) {
    Vec2 child_offset = {};
    Vec2 consumed_size = {};
    Vec2 max_size = {};

    u32 flex_element_count = 0;
    float flex_total = 0.0f;
    u32 element_stack_start = g_ui.element_stack_count;

    // Get spacing from parent style
    float spacing = 0.0f;
    if (parent->type == ELEMENT_TYPE_ROW) {
        spacing = static_cast<RowElement*>(parent)->style.spacing;
    } else if (parent->type == ELEMENT_TYPE_COLUMN) {
        spacing = static_cast<ColumnElement*>(parent)->style.spacing;
    }

    Vec2 constraints = size;
    if (parent->type == ELEMENT_TYPE_ROW) {
        constraints.x = F32_MAX;
    } else if (parent->type == ELEMENT_TYPE_COLUMN) {
        constraints.y = F32_MAX;
    } else if (parent->type == ELEMENT_TYPE_ALIGN || parent->type == ELEMENT_TYPE_CENTER) {
        constraints.x = F32_MAX;
        constraints.y = F32_MAX;
    }

    for (u32 i = 0; i < parent->child_count; i++) {
        Element* child = g_ui.elements[element_index++];
        g_ui.element_stack[g_ui.element_stack_count++] = child;

        if (child->type == ELEMENT_TYPE_EXPANDED) {
            flex_element_count++;
            flex_total += static_cast<ExpandedElement*>(child)->style.flex;
            element_index = SkipElement(element_index - 1);
            continue;
        }

        element_index = LayoutElement(element_index - 1, constraints, parent);

        EdgeInsets child_margin = GetMargin(child);

        // ALIGN/CENTER children handle their own margins internally, so don't add margin offsets
        bool child_handles_own_margins = (child->type == ELEMENT_TYPE_ALIGN || child->type == ELEMENT_TYPE_CENTER);

        // For ALIGN/CENTER parents, don't add margins yet - ApplyAlignment will handle positioning
        if (parent->type == ELEMENT_TYPE_ALIGN || parent->type == ELEMENT_TYPE_CENTER) {
            child->rect.x = 0;
            child->rect.y = 0;
        } else if (child_handles_own_margins) {
            child->rect.x = child_offset.x;
            child->rect.y = child_offset.y;
        } else {
            child->rect.x = child_offset.x + child_margin.left;
            child->rect.y = child_offset.y + child_margin.top;
        }

        // ALIGN/CENTER elements already include their margins in their rect size
        float child_total_width = child_handles_own_margins ? child->rect.width : (child->rect.width + child_margin.left + child_margin.right);
        float child_total_height = child_handles_own_margins ? child->rect.height : (child->rect.height + child_margin.top + child_margin.bottom);

        if (parent->type == ELEMENT_TYPE_ROW) {
            child_offset.x += child_total_width;
            consumed_size.x += child_total_width;
            // Add spacing between children (but not after the last child)
            if (i < parent->child_count - 1 - flex_element_count) {
                child_offset.x += spacing;
                consumed_size.x += spacing;
            }
        } else if (parent->type == ELEMENT_TYPE_COLUMN) {
            child_offset.y += child_total_height;
            consumed_size.y += child_total_height;
            // Add spacing between children (but not after the last child)
            if (i < parent->child_count - 1 - flex_element_count) {
                child_offset.y += spacing;
                consumed_size.y += spacing;
            }
        }

        max_size = {
            Max(max_size.x, child_total_width),
            Max(max_size.y, child_total_height)
        };
    }

    // For Row/Column, use consumed size instead of max size
    if (parent->type == ELEMENT_TYPE_ROW) {
        max_size.x = consumed_size.x;
    } else if (parent->type == ELEMENT_TYPE_COLUMN) {
        max_size.y = consumed_size.y;
    }

    if (flex_element_count > 0 && parent->type == ELEMENT_TYPE_ROW && size.x != F32_MAX) {
        float remaining_width = size.x - consumed_size.x;
        float offset = 0.0f;
        for (u32 i = 0; i < parent->child_count; i++) {
            Element* child = g_ui.element_stack[element_stack_start + i];
            if (child->type != ELEMENT_TYPE_EXPANDED) {
                EdgeInsets child_margin = GetMargin(child);
                bool child_handles_own_margins = (child->type == ELEMENT_TYPE_ALIGN || child->type == ELEMENT_TYPE_CENTER);
                child->rect.x += offset;
                float child_total_width = child_handles_own_margins ? child->rect.width : (child->rect.width + child_margin.right);
                child_offset.x = child->rect.x + child_total_width;
                if (i < parent->child_count - 1) {
                    child_offset.x += spacing;
                    offset += spacing;
                }
                continue;
            }

            ExpandedElement* expanded = static_cast<ExpandedElement*>(child);
            EdgeInsets expanded_margin = GetMargin(expanded);
            Vec2 expanded_size = { remaining_width * (expanded->style.flex / flex_total), size.y };
            LayoutElement(child->index, expanded_size, parent);

            child->rect.x = child_offset.x + expanded_margin.left;
            child->rect.y = expanded_margin.top;

            child_offset.x += expanded_size.x + expanded_margin.left + expanded_margin.right;
            offset += expanded_size.x + expanded_margin.left + expanded_margin.right;
            if (i < parent->child_count - 1) {
                child_offset.x += spacing;
                offset += spacing;
            }
        }

        max_size.x = child_offset.x;
    } else if (flex_element_count > 0 && parent->type == ELEMENT_TYPE_COLUMN && size.y != F32_MAX) {
        float remaining_height = size.y - consumed_size.y;
        float offset = 0.0f;
        for (u32 i = 0; i < parent->child_count; i++) {
            Element* child = g_ui.element_stack[element_stack_start + i];
            if (child->type != ELEMENT_TYPE_EXPANDED) {
                EdgeInsets child_margin = GetMargin(child);
                bool child_handles_own_margins = (child->type == ELEMENT_TYPE_ALIGN || child->type == ELEMENT_TYPE_CENTER);
                child->rect.y += offset;
                float child_total_height = child_handles_own_margins ? child->rect.height : (child->rect.height + child_margin.bottom);
                child_offset.y = child->rect.y + child_total_height;
                if (i < parent->child_count - 1) {
                    child_offset.y += spacing;
                    offset += spacing;
                }
                continue;
            }

            ExpandedElement* expanded = static_cast<ExpandedElement*>(child);
            EdgeInsets expanded_margin = GetMargin(expanded);
            Vec2 expanded_size = { size.x, remaining_height * (expanded->style.flex / flex_total) };
            LayoutElement(child->index, expanded_size, parent);

            child->rect.x = expanded_margin.left;
            child->rect.y = child_offset.y + expanded_margin.top;

            child_offset.y += expanded_size.y + expanded_margin.top + expanded_margin.bottom;
            offset += expanded_size.y + expanded_margin.top + expanded_margin.bottom;
            if (i < parent->child_count - 1) {
                child_offset.y += spacing;
                offset += spacing;
            }
        }

        max_size.y = child_offset.y;
    }

    if (parent->rect.width == F32_MAX)
        parent->rect.width = max_size.x;
    if (parent->rect.height == F32_MAX)
        parent->rect.height = max_size.y;

    if (parent->type == ELEMENT_TYPE_CENTER) {
        ApplyAlignment(parent, ALIGNMENT_CENTER, element_stack_start, parent->child_count);
    } else if (parent->type == ELEMENT_TYPE_ALIGN) {
        AlignElement* align = static_cast<AlignElement*>(parent);
        ApplyAlignment(parent, align->style.alignment, element_stack_start, parent->child_count);
    } else if (parent->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(parent);
        for (u32 i = 0; i < parent->child_count; i++) {
            Element* child = g_ui.element_stack[element_stack_start + i];
            child->rect.x += container->style.padding.left;
            child->rect.y += container->style.padding.top;
        }
    } else if (parent->type == ELEMENT_TYPE_BORDER) {
        BorderElement* border = static_cast<BorderElement*>(parent);
        for (u32 i = 0; i < parent->child_count; i++) {
            Element* child = g_ui.element_stack[element_stack_start + i];
            child->rect.x += border->style.width;
            child->rect.y += border->style.width;
        }
    }

    g_ui.element_stack_count-=parent->child_count;

    return element_index;
}

static int LayoutElement(int element_index, const Vec2& constraints, Element* ) {
    Element* e = g_ui.elements[element_index++];
    assert(e);

    Vec2 margin_constrained = constraints;
    EdgeInsets margin = {};

    if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        margin = container->style.margin;
    } else if (e->type == ELEMENT_TYPE_ALIGN) {
        AlignElement* align = static_cast<AlignElement*>(e);
        margin = align->style.margin;
    }

    // For ALIGN/CENTER, don't reduce rect size by margins - they need the full size for alignment
    // The parent will position them with margin offsets
    if (e->type != ELEMENT_TYPE_ALIGN && e->type != ELEMENT_TYPE_CENTER) {
        if (margin_constrained.x != F32_MAX)
            margin_constrained.x -= margin.left + margin.right;
        if (margin_constrained.y != F32_MAX)
            margin_constrained.y -= margin.top + margin.bottom;
    }

    e->rect.width = margin_constrained.x;
    e->rect.height = margin_constrained.y;

    if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        e->rect.width = Min(container->style.width, e->rect.width);
        e->rect.height = Min(container->style.height, e->rect.height);
    } else if (e->type == ELEMENT_TYPE_SIZED_BOX) {
        SizedBoxElement* sized_box = static_cast<SizedBoxElement*>(e);
        e->rect.width = Min(sized_box->style.width, e->rect.width);
        e->rect.height = Min(sized_box->style.height, e->rect.height);
    } else if (e->type == ELEMENT_TYPE_LABEL) {
        LabelElement* l = static_cast<LabelElement*>(e);
        if (l->cached_mesh && l->cached_mesh->text_mesh) {
            Vec2 text_size = GetSize(l->cached_mesh->text_mesh);
            if (e->rect.width == F32_MAX) e->rect.width = text_size.x;
            if (e->rect.height == F32_MAX) e->rect.height = text_size.y;

            if (text_size.x < e->rect.width)
                l->offset.x = (e->rect.width - text_size.x) * 0.5f * (l->style.align.x + 1.0f);
            if (text_size.y < e->rect.height)
                l->offset.y = (e->rect.height - text_size.y) * 0.5f * (l->style.align.y + 1.0f);
        }
    }

    if (e->child_count == 0) {
        if (e->rect.width == F32_MAX) e->rect.width = 0;
        if (e->rect.height == F32_MAX) e->rect.height = 0;
        return element_index;
    }

    Vec2 child_constraints = GetSize(e->rect);
    if (e->type == ELEMENT_TYPE_CONTAINER) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        if (child_constraints.x != F32_MAX) child_constraints.x -= (container->style.padding.left  + container->style.padding.right);
        if (child_constraints.y != F32_MAX) child_constraints.y -= (container->style.padding.top   + container->style.padding.bottom);
    } else if (e->type == ELEMENT_TYPE_BORDER) {
        BorderElement* border = static_cast<BorderElement*>(e);
        if (child_constraints.x != F32_MAX) child_constraints.x -= border->style.width * 2.0f;
        if (child_constraints.y != F32_MAX) child_constraints.y -= border->style.width * 2.0f;
    }

    element_index = LayoutChildren(element_index, e, child_constraints);

    if (e->rect.width == F32_MAX) e->rect.width = 0;
    if (e->rect.height == F32_MAX) e->rect.height = 0;

    return element_index;
}

static int LayoutCanvas(int element_index) {
    Element* e = g_ui.elements[element_index];
    assert(e);
    assert(e->type == ELEMENT_TYPE_CANVAS);

    CanvasElement* c = static_cast<CanvasElement*>(e);

    Vec2 contraints = g_ui.ortho_size;
    if (c->style.type == CANVAS_TYPE_WORLD) {
        Vec2 screen_pos = WorldToScreen(c->style.world_camera, c->style.world_position);
        Vec2 screen_size = Abs(WorldToScreen(c->style.world_camera, c->style.world_size) - WorldToScreen(c->style.world_camera, VEC2_ZERO));
        screen_pos = screen_pos - screen_size * 0.5f;

        Vec2 ui_pos = ScreenToWorld(g_ui.camera, screen_pos);
        Vec2 ui_size = ScreenToWorld(g_ui.camera, screen_size) - ScreenToWorld(g_ui.camera, VEC2_ZERO);
        e->rect.x = ui_pos.x;
        e->rect.y = ui_pos.y;
        contraints = ui_size;
    }

    return LayoutElement(element_index, contraints, nullptr);
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

static void RenderBackground(const Rect& rect, const Mat3& transform, const Color& color) {
    if (color.a <= 0.0f)
        return;

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
        Bounds2 mesh_bounds = GetBounds(image->mesh);
        Vec2 mesh_size = GetSize(mesh_bounds);
        Vec2 mesh_scale = Vec2 {e->rect.width / mesh_size.x, e->rect.height / mesh_size.y};
        BindTransform(transform * Translate(-mesh_bounds.min * mesh_scale) * Scale(mesh_scale) * Scale(Vec2{1, -1}));
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
            BindColor(rectangle->style.color_func(e->state, 0.0f, rectangle->style.color_func_user_data));
        } else {
            BindColor(rectangle->style.color);
        }
        DrawMesh(g_ui.element_quad);
    } else if (e->type == ELEMENT_TYPE_BORDER) {
        BorderElement* border = static_cast<BorderElement*>(e);
        float border_width = border->style.width;
        BindColor(border->style.color);
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

static void HandleInput()
{
    Vec2 mouse = ScreenToWorld(g_ui.camera, GetMousePosition());
    for (u32 i=g_ui.element_count; i>0; i--)
    {
        Element* e = g_ui.elements[i-1];
        ElementState prev_state = g_ui.prev_element_states[i-1];
        Vec2 local_mouse = TransformPoint(e->world_to_local, mouse);
        bool mouse_over = Contains(Bounds2{0,0,e->rect.width, e->rect.height}, local_mouse);

        if (mouse_over)
            e->state = e->state | ELEMENT_STATE_HOVERED;
        else
            e->state = e->state & ~ELEMENT_STATE_HOVERED;

        if (e->type == ELEMENT_TYPE_GESTURE_DETECTOR) {
            GestureDetectorElement* g = static_cast<GestureDetectorElement*>(e);
            if (mouse_over && g->style.on_tap && WasButtonPressed(g_ui.input, MOUSE_LEFT)) {
                TapDetails details = {.position = local_mouse};
                g->style.on_tap(details, g->style.user_data);
                ConsumeButton(MOUSE_LEFT);
            }
        } else if (e->type == ELEMENT_TYPE_MOUSE_REGION) {
            MouseRegionElement* m = static_cast<MouseRegionElement*>(e);
            bool was_hovered = prev_state & ELEMENT_STATE_HOVERED;
            bool is_hovered = e->state & ELEMENT_STATE_HOVERED;

            // Trigger on_enter when transitioning from not-hovered to hovered
            if (!was_hovered && is_hovered && m->style.on_enter) {
                m->style.on_enter();
            }

            // Trigger on_exit when transitioning from hovered to not-hovered
            if (was_hovered && !is_hovered && m->style.on_exit) {
                m->style.on_exit();
            }

            // Trigger on_hover every frame while hovered
            if (is_hovered && m->style.on_hover) {
                m->style.on_hover();
            }
        }

        // Store current state for next frame
        g_ui.prev_element_states[i-1] = e->state;
    }
}

void EndUI()
{
    for (u32 element_index=0; element_index < g_ui.element_count; )
        element_index = LayoutCanvas(element_index);
    for (u32 element_index=0; element_index < g_ui.element_count; )
        element_index = CalculateTransforms(element_index, MAT3_IDENTITY);

    HandleInput();
}

void DrawUI() {
    BindDepth(10.0f);
    for (u32 element_index = 0; element_index < g_ui.element_count; )
        element_index = RenderElement(element_index);
    BindDepth(0.0f);
}

static Mesh* CreateElementQuad(Allocator* allocator) {
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

static Mesh* CreateImageQuad(Allocator* allocator) {
    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(builder, { -0.5f, -0.5f }, { 0.0f, 1.0f }, { 0.0f, 1.0f });
    AddVertex(builder, {  0.5f, -0.5f }, { 0.0f, 1.0f }, { 1.0f, 1.0f });
    AddVertex(builder, {  0.5f,  0.5f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
    AddVertex(builder, { -0.5f,  0.5f }, { 0.0f, 1.0f }, { 0.0f, 0.0f });
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);
    auto mesh = CreateMesh(allocator, builder, GetName("image"));
    PopScratch();
    return mesh;
}

void InitUI() {
    g_ui = {};
    g_ui.allocator = CreateArenaAllocator(sizeof(FatElement) * MAX_ELEMENTS, "UI");
    g_ui.camera = CreateCamera(ALLOCATOR_DEFAULT);
    g_ui.element_quad = CreateElementQuad(ALLOCATOR_DEFAULT);
    g_ui.image_mesh = CreateImageQuad(ALLOCATOR_DEFAULT);
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

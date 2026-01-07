//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#include "../platform.h"

constexpr int MAX_ELEMENTS = 4096;
constexpr int MAX_ELEMENT_STACK = 128;
constexpr int MAX_TEXT_MESHES = 4096;
constexpr int MAX_POPUPS = 4;

extern void UpdateInputState(InputSet* input_set);
extern void UpdateDebugUI();

enum ElementType : u8 {
    ELEMENT_TYPE_UNKNOWN = 0,
    ELEMENT_TYPE_NONE,
    ELEMENT_TYPE_CANVAS,
    ELEMENT_TYPE_COLUMN,
    ELEMENT_TYPE_CONTAINER,
    ELEMENT_TYPE_EXPANDED,
    ELEMENT_TYPE_GRID,
    ELEMENT_TYPE_IMAGE,
    ELEMENT_TYPE_LABEL,
    ELEMENT_TYPE_ROW,
    ELEMENT_TYPE_SCENE,
    ELEMENT_TYPE_SCROLLABLE,
    ELEMENT_TYPE_SPACER,
    ELEMENT_TYPE_TRANSFORM,
    ELEMENT_TYPE_TEXTBOX,
    ELEMENT_TYPE_POPUP,
    ELEMENT_TYPE_COUNT
};

struct AlignInfo {
    bool has_x;
    float x;
    bool has_y;
    float y;
};

inline bool IsAuto(float v) { return v >= F32_AUTO; }
inline bool IsFixed(float v) { return !IsAuto(v); }
inline bool IsContainerType(ElementType type) {
    return type == ELEMENT_TYPE_CONTAINER ||
        type == ELEMENT_TYPE_COLUMN ||
        type == ELEMENT_TYPE_ROW;
}

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
    u16 index;
    CanvasId canvas_id;
    Text text;
    float scroll_offset;
    noz::Rect rect;  // Last known rect (for GetElementRect)
};

struct Element {
    ElementType type;
    ElementId id;
    CanvasId canvas_id;
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
    Vec2 position;
};

struct ContainerElement : Element {
    ContainerStyle style;
};

struct GridElement : Element {
    GridStyle style;
    int start_row;
};

struct ExpandedElement : Element {
    ExpandedStyle style;
    int axis;
};

struct LabelElement : Element {
    LabelStyle style;
    CachedTextMesh* cached_mesh = nullptr;
};

struct ImageElement : Element {
    ImageStyle style;
    Mesh* mesh = nullptr;
    Texture* texture;
    float animated_time = 0.0f;
};

struct PopupElement : Element {
    PopupStyle style;
};

struct SceneElement : Element {
    SceneStyle style;
    void (*draw_scene)(void*);
};

struct ScrollableElement : Element {
    ScrollableStyle style;
    float offset;
    float content_height;
};

struct SpacerElement : Element {
    Vec2 size;
};

struct TransformElement : Element {
    TransformStyle style;
};

struct TextboxElement : Element {
    TextBoxStyle style;
};

union FatElement {
    Element element;
    CanvasElement canvas;
    ContainerElement container;
    ExpandedElement expanded;
    LabelElement label;
    ImageElement image;
    SpacerElement spacer;
    TextboxElement textbox;
    GridElement grid;
};

struct UI {
    Allocator* allocator;
    Camera* camera;
    Camera* cursor_camera;
    Element* elements[MAX_ELEMENTS];
    Element* element_stack[MAX_ELEMENTS];
    Element* popups[MAX_POPUPS];
    ElementState element_states[ELEMENT_ID_MAX + 1];
    u16 element_count;
    u16 element_stack_count;
    u16 popup_count;
    ElementId focus_id;
    ElementId pending_focus_id;
    ElementId textbox_id;
    Mesh* element_mesh;
    Mesh* image_element_mesh;
    Mesh* element_with_border_mesh;
    Material* element_material;
    Material* image_element_material;
    Vec2 ortho_size;
    Vec2Int ref_size;
    InputSet* input;
    PoolAllocator* text_mesh_allocator;
    float depth;
    Text password_mask;
    CanvasId current_canvas_id;
    CanvasId current_focus_canvas_id;
    ElementId active_scroll_id;
    float last_scroll_mouse_y;
    bool close_popups;
};

static UI g_ui;

static int LayoutElement(int element_index, const Vec2& size);

CanvasId GetFocusedCanvasId() {
    return g_ui.current_focus_canvas_id;
}

extern ElementId GetFocusedElementId() {
    return g_ui.focus_id;
}

extern noz::Rect GetElementRect(ElementId id) {
    if (id == ELEMENT_ID_NONE) return {};
    return g_ui.element_states[id].rect;
}

static void SetId(Element* e, ElementId id) {
    if (id == ELEMENT_ID_NONE)
        return;
    if (g_ui.current_canvas_id != g_ui.current_focus_canvas_id)
        return;

    e->id = id;
    ElementState& state = g_ui.element_states[id];
    state.index = e->index;
    state.canvas_id = g_ui.current_focus_canvas_id;
}

static Element* CreateElement(ElementType type) {
    Element* element = static_cast<Element*>(Alloc(g_ui.allocator, sizeof(FatElement)));
    element->index = g_ui.element_count;
    element->type = type;
    element->canvas_id = g_ui.current_canvas_id;
    g_ui.elements[element->index] = element;
    g_ui.element_count++;

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

static void CommitTextBox() {
    if (!g_ui.textbox_id)
        return;

    PlatformHideTextbox();
    g_ui.textbox_id = 0;
}

bool HasFocus() {
    return g_ui.focus_id != 0 && GetCurrentElement()->id == g_ui.focus_id;
}

static void SetFocus(ElementId focus_id) {
    if (g_ui.focus_id == focus_id)
        return;

    if (g_ui.textbox_id != focus_id)
        CommitTextBox();

    g_ui.focus_id = focus_id;
    g_ui.pending_focus_id = focus_id;
}

static void SetPendingFocus(ElementId focus_id) {
    g_ui.pending_focus_id = focus_id;
}

void SetFocus(CanvasId canvas_id, ElementId element_id) {
    // Canvas id changing?  if so reset the states
    if (canvas_id != g_ui.current_focus_canvas_id) {
        memset(&g_ui.element_states, 0, sizeof(g_ui.element_states));
        g_ui.current_focus_canvas_id = canvas_id;
    }

    SetFocus(element_id);
}

Vec2 ScreenToElement(const Vec2& screen) {
    Element* e = GetCurrentElement();
    if (!e) return VEC2_ZERO;
    return TransformPoint(e->world_to_local, ScreenToWorld(g_ui.camera, screen));
}

ElementId GetElementId() {
    Element* e = GetCurrentElement();
    if (!e) return 0;
    return e->id;
}

bool CheckElementFlags(ElementFlags flags) {
    if (g_ui.element_stack_count <= 0)
        return false;

    Element* e = g_ui.element_stack[g_ui.element_stack_count - 1];
    assert(e);
    if (e->id == ELEMENT_ID_NONE)
        return false;

    return (g_ui.element_states[e->id].flags & flags) == flags;
}

Vec2 ScreenToUI(const Vec2& screen_pos) {
    return screen_pos / ToVec2(GetScreenSize()) * g_ui.ortho_size;
}

Vec2 GetUISize() {
    return g_ui.ortho_size;
}

static float GetUIScale() {
    return ToVec2(GetScreenSize()).y / g_ui.ortho_size.y;
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
    (void)expected_type;
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

void BeginColumn(const ContainerStyle& style) {
    ContainerElement* column = static_cast<ContainerElement*>(CreateElement(ELEMENT_TYPE_COLUMN));
    column->style = static_cast<ContainerStyle>(style);
    PushElement(column);
}

void EndColumn() {
    EndElement(ELEMENT_TYPE_COLUMN);
}

void BeginPopup(const PopupStyle& style) {
    PopupElement* e = static_cast<PopupElement*>(CreateElement(ELEMENT_TYPE_POPUP));
    e->style = style;
    PushElement(e);

    g_ui.popups[g_ui.popup_count++] = e;
}

void EndPopup() {
    EndElement(ELEMENT_TYPE_POPUP);
}

static float GetFixedParentHeight() {
    for (int i = g_ui.element_stack_count - 1; i >= 0; i--) {
        Element* parent = g_ui.element_stack[i];

        if (IsContainerType(parent->type)) {
            ContainerElement* container = static_cast<ContainerElement*>(parent);
            if (IsFixed(container->style.height)) {
                return container->style.height;
            }
        }
    }

    return F32_AUTO;
}

// @grid
void BeginGrid(const GridStyle& style) {
    GridElement* grid = static_cast<GridElement*>(CreateElement(ELEMENT_TYPE_GRID));

    grid->style = style;
    grid->start_row = 0;
    PushElement(grid);

    // If no virtual items, skip virtualization
    if (style.virtual_count == 0)
        return;

    float container_height = GetFixedParentHeight();
    if (IsAuto(container_height))
        return;

    float row_height = style.cell.height + style.spacing;
    float scroll_offset = GetScrollOffset(style.scroll_id);

    int start_row = Max(0, FloorToInt(scroll_offset / row_height));
    int visible_rows = CeilToInt(container_height / row_height) + 1; // +1 for partial row
    int end_row = start_row + visible_rows;

    int start_index = start_row * style.columns;
    int end_index = Min(end_row * style.columns, style.virtual_count);

    grid->start_row = start_row;

    if (style.virtual_range_func)
        style.virtual_range_func(start_index, end_index);

    for (int virtual_index = start_index; virtual_index < end_index; virtual_index++) {
        int cell_index = virtual_index - start_index;
        if (style.virtual_cell_func)
            style.virtual_cell_func(cell_index, virtual_index);
    }
}

void EndGrid() {
    EndElement(ELEMENT_TYPE_GRID);
}

// @scrollable
float BeginScrollable(float offset, const ScrollableStyle& style) {
    ScrollableElement* scrollable = static_cast<ScrollableElement*>(CreateElement(ELEMENT_TYPE_SCROLLABLE));
    scrollable->style = style;
    scrollable->content_height = 0;
    SetId(scrollable, style.id);

    // Get persisted scroll offset from ElementState, or use provided offset
    if (style.id != ELEMENT_ID_NONE) {
        ElementState& state = g_ui.element_states[style.id];
        scrollable->offset = state.scroll_offset;
    } else {
        scrollable->offset = offset;
    }

    PushElement(scrollable);
    return scrollable->offset;
}

void EndScrollable() {
    EndElement(ELEMENT_TYPE_SCROLLABLE);
}

float GetScrollOffset(ElementId id) {
    if (id == ELEMENT_ID_NONE) return 0.0f;
    return g_ui.element_states[id].scroll_offset;
}

// @canvas
void BeginCanvas(const CanvasStyle& style) {
    CanvasElement* canvas = static_cast<CanvasElement*>(CreateElement(ELEMENT_TYPE_CANVAS));
    canvas->style = style;
    g_ui.current_canvas_id = style.id;
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
    SetId(e, style.id);
}

void EndContainer() {
    EndElement(ELEMENT_TYPE_CONTAINER);
}

void Container(const ContainerStyle& style) {
    BeginContainer(style);
    EndContainer();
}

// @row
void BeginRow(const ContainerStyle& style) {
    ContainerElement* row = static_cast<ContainerElement*>(CreateElement(ELEMENT_TYPE_ROW));
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
    ExpandedElement* e = static_cast<ExpandedElement*>(CreateElement(ELEMENT_TYPE_EXPANDED));
    e->style = style;
    e->axis = parent->type == ELEMENT_TYPE_ROW ? 0 : 1;
    PushElement(e);
}

void EndExpanded() {
    assert(GetCurrentElement()->type == ELEMENT_TYPE_EXPANDED);
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
    SpacerElement* e = static_cast<SpacerElement*>(CreateElement(ELEMENT_TYPE_SPACER));
    e->size = {};
    e->size[parent->type == ELEMENT_TYPE_ROW ? 0 : 1] = size;
}


static u64 GetMeshHash(const TextRequest& request) {
    return Hash(Hash(request.text), reinterpret_cast<u64>(request.font), static_cast<u64>(request.font_size));
}

static CachedTextMesh* GetOrCreateTextMesh(const char* text, const LabelStyle& style) {
    TextRequest r = {};
    r.font = style.font;
    r.font_size = style.font_size;
    Set(r.text, text);

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

    if (!label->style.font)
        label->style.font = FONT_DEFAULT;
}

void Image(Texture* texture, const ImageStyle& style) {
    ImageElement* image = static_cast<ImageElement*>(CreateElement(ELEMENT_TYPE_IMAGE));
    image->texture = texture;
    image->style = style;
}

void Image(Mesh* mesh, const ImageStyle& style) {
    ImageElement* image = static_cast<ImageElement*>(CreateElement(ELEMENT_TYPE_IMAGE));
    image->mesh = mesh;
    image->style = style;
    if (!image->style.material)
        image->style.material = g_ui.image_element_material;
}

void Image(Mesh* mesh, float time, const ImageStyle& style) {
    ImageElement* image = static_cast<ImageElement*>(CreateElement(ELEMENT_TYPE_IMAGE));
    image->mesh = mesh;
    image->animated_time = time;
    image->style = style;
    if (!image->style.material)
        image->style.material = g_ui.image_element_material;
}

void Rectangle(const RectangleStyle& style) {
    ContainerElement* e = static_cast<ContainerElement*>(CreateElement(ELEMENT_TYPE_CONTAINER));
    e->style = {};
    e->style.width = style.width;
    e->style.height = style.height;
    e->style.color = style.color;
}

bool TextBox(Text& text, const TextBoxStyle& style) {
    BeginContainer({.height=style.height, .id=style.id, .nav=style.nav});
    ElementId id = GetElementId();
    bool text_changed = false;

    BeginContainer({
        .padding=EdgeInsetsAll(style.padding),
        .color=style.background_color,
        .border=HasFocus() ? style.focus_border : style.border
    });
    {
        TextboxElement* textbox = static_cast<TextboxElement*>(CreateElement(ELEMENT_TYPE_TEXTBOX));
        textbox->id = id;
        textbox->style = style;
        Set(g_ui.element_states[id].text, text);

        if (g_ui.focus_id == id)
            text_changed = PlatformUpdateTextboxText(text);

        if (id == 0 || g_ui.textbox_id != id) {
            if (text.value[0] != '\0') {
                // password
                if (style.password) {
                    g_ui.password_mask.value[text.length] = 0;
                    g_ui.password_mask.length = text.length;
                    Label(g_ui.password_mask, {
                        .font=style.font,
                        .font_size=style.font_size,
                        .color=style.text_color,
                        .align=ALIGN_CENTER_LEFT});
                    g_ui.password_mask.value[text.length] = '*';
                    // default
                } else {
                    Label(text, {
                        .font=style.font,
                        .font_size=style.font_size,
                        .color=style.text_color,
                        .align=ALIGN_CENTER_LEFT});
                }
            // placeholder
            } else if (style.placeholder) {
                Label(style.placeholder, {
                    .font=style.font,
                    .font_size=style.font_size,
                    .color=style.placeholder_color,
                    .align=ALIGN_CENTER_LEFT});
            } else {
                Container({});
            }
        } else {
            Container({});
        }
    }

    EndContainer();
    EndContainer();

    return text_changed;
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

static int MeasureElement(int element_index, const Vec2& available_size);

static int LayoutGrid(int element_index, const Vec2& size) {
    GridElement* e = static_cast<GridElement*>(g_ui.elements[element_index++]);

    int columns = e->style.columns;
    int start_index = e->start_row * columns;

    for (u16 i = 0; i < e->child_count; i++) {
        int virtual_index = start_index + i;
        int col = virtual_index % columns;
        int row = virtual_index / columns;

        Element* child = g_ui.elements[element_index];
        element_index = LayoutElement(element_index, size);
        child->rect.x = col * (e->style.cell.width + e->style.spacing);
        child->rect.y = row * (e->style.cell.height + e->style.spacing);
    }

    return element_index;
}

static int MeasureGrid(int element_index, const Vec2& available_size) {
    GridElement* e = static_cast<GridElement*>(g_ui.elements[element_index++]);

    float requested_width = e->style.columns * e->style.cell.width +
        e->style.spacing * (static_cast<float>(e->style.columns) - 1.0f);

    Vec2 availble_child_size = {e->style.cell.width, e->style.cell.height };

    int columns = e->style.columns;
    float max_column_width = available_size.x / static_cast<float>(e->style.columns);
    max_column_width -= e->style.spacing * (static_cast<float>(columns) - 1.0f) / static_cast<float>(columns);

    if (availble_child_size.x < e->style.cell.width) {
        float scale = max_column_width / availble_child_size.x;
        availble_child_size.x = max_column_width;
        availble_child_size.y *= scale;
    }

    for (u16 i = 0; i < e->child_count; i++)
        element_index = MeasureElement(element_index, availble_child_size);

    int row_count = e->style.virtual_cell_func
        ? (e->style.virtual_count + columns - 1) / columns
        : (e->child_count + columns - 1) / columns;

    e->measured_size.x = requested_width;
    e->measured_size.y = row_count * availble_child_size.y +
        e->style.spacing * (static_cast<float>(row_count) - 1.0f);

    return element_index;
}

static int MeasureRowColumnContent(int element_index, const Vec2& available_size, int axis, int cross_axis, Vec2& max_content_size) {
    ContainerElement* e = static_cast<ContainerElement*>(g_ui.elements[element_index++]);
    int child_element_index = element_index;
    float flex_total = 0.0f;
    for (u16 i = 0; i < e->child_count; i++) {
        Element* child = g_ui.elements[element_index];
        if (child->type == ELEMENT_TYPE_EXPANDED) {
            Vec2 child_available_size = {};
            child_available_size[cross_axis] = available_size[cross_axis];
            element_index = MeasureElement(element_index, child_available_size);
        } else {
            element_index = MeasureElement(element_index, available_size);
        }
        max_content_size[cross_axis] = Max(
            max_content_size[cross_axis],
            child->measured_size[cross_axis]);
        max_content_size[axis] += child->measured_size[axis];
        if (child->type == ELEMENT_TYPE_EXPANDED)
            flex_total += static_cast<ExpandedElement*>(child)->style.flex;
    }

    float spacing = e->child_count > 1 ? e->style.spacing * (e->child_count - 1) : 0.0f;

    if (flex_total >= F32_EPSILON) {
        float flex_available = Max(0.0f, available_size[axis] - max_content_size[axis]) - spacing;
        max_content_size[axis] = Max(max_content_size[axis], available_size[axis]);
        Vec2 child_available_size = {};
        child_available_size[cross_axis] = max_content_size[cross_axis];
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[child_element_index];
            if (child->type == ELEMENT_TYPE_EXPANDED) {
                ExpandedElement* expanded = static_cast<ExpandedElement*>(child);
                child_available_size[axis] = expanded->style.flex / flex_total * flex_available;
                MeasureElement(child_element_index, child_available_size);
            }
            child_element_index = child->next_sibling_index;
        }
    }

    if (e->child_count > 1)
        max_content_size[axis] += spacing;

    return element_index;
}

static int MeasureElement(int element_index, const Vec2& available_size) {
    Element* e = g_ui.elements[element_index++];
    assert(e);

    // @measure_container
    if (IsContainerType(e->type)) {
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

        if (e->type == ELEMENT_TYPE_CONTAINER) {
            for (u16 i = 0; i < e->child_count; i++) {
                Element* child = g_ui.elements[element_index];
                element_index = MeasureElement(element_index, content_size);
                max_content_size = Max(max_content_size, child->measured_size);
            }
        } else if (e->type == ELEMENT_TYPE_ROW) {
            element_index = MeasureRowColumnContent(element_index - 1, content_size, 0, 1, max_content_size);
        } else if (e->type == ELEMENT_TYPE_COLUMN) {
            element_index = MeasureRowColumnContent(element_index - 1, content_size, 1, 0, max_content_size);
        }

        if (!is_auto_width)
            e->measured_size.x = container->style.width;
        else
            e->measured_size.x = Min(available_size.x,
                max_content_size.x +
                container->style.padding.left +
                container->style.padding.right +
                container->style.border.width * 2.0f);

        if (!is_auto_height)
            e->measured_size.y = container->style.height;
        else
            e->measured_size.y = Min(available_size.y,
                max_content_size.y +
                container->style.padding.top +
                container->style.padding.bottom +
                container->style.border.width * 2.0f);

        // Apply min size constraints
        e->measured_size.x = Max(e->measured_size.x, container->style.min_width);
        e->measured_size.y = Max(e->measured_size.y, container->style.min_height);

    // @measure_label
    } else if (e->type == ELEMENT_TYPE_LABEL) {
        LabelElement* l = static_cast<LabelElement*>(e);
        if (l->cached_mesh && l->cached_mesh->text_mesh) {
            Vec2 text_size = GetSize(l->cached_mesh->text_mesh);
            e->measured_size = text_size;
        }

        assert(e->child_count == 0);

    // @measure_image
    } else if (e->type == ELEMENT_TYPE_IMAGE) {
        ImageElement* i = static_cast<ImageElement*>(e);
        if (i->mesh) {
            e->measured_size = GetSize(i->mesh) * i->style.scale * 72.0f;
        } else if (i->texture) {
            e->measured_size = ToVec2(GetSize(i->texture)) * i->style.scale;
        } else {
            e->measured_size = VEC2_ZERO;
        }

        assert(e->child_count == 0);

    // @measure_canvas
    } else if (e->type == ELEMENT_TYPE_CANVAS) {
        CanvasElement* canvas = static_cast<CanvasElement*>(e);
        if (canvas->style.type == CANVAS_TYPE_SCREEN) {
            e->measured_size = available_size;

        } else if (canvas->style.type == CANVAS_TYPE_WORLD) {
            Vec2 screen_pos = WorldToScreen(canvas->style.world_camera, canvas->style.world_position);
            Vec2 screen_size = Abs(WorldToScreen(canvas->style.world_camera, canvas->style.world_size) - WorldToScreen(canvas->style.world_camera, VEC2_ZERO));
            screen_pos = screen_pos - screen_size * 0.5f;

            Vec2 ui_pos = ScreenToWorld(g_ui.camera, screen_pos);
            Vec2 ui_size = ScreenToWorld(g_ui.camera, screen_size) - ScreenToWorld(g_ui.camera, VEC2_ZERO);
            canvas->position = ui_pos;
            e->measured_size = ui_size;

        } else {
            assert(false && "Unknown canvas type");
        }

        for (u16 i = 0; i < e->child_count; i++)
            element_index = MeasureElement(element_index, e->measured_size);

    // @measure_scene
    } else if (e->type == ELEMENT_TYPE_SCENE) {
        SceneElement* scene = static_cast<SceneElement*>(e);
        if (scene->style.camera) {
            Update(scene->style.camera, Vec2Int{
                static_cast<i32>(available_size.x),
                static_cast<i32>(available_size.y)});
            e->measured_size = ToVec2(GetScreenSize(scene->style.camera));
        } else {
            e->measured_size = available_size;
        }

    // @measure_spacer_row
    } else if (e->type == ELEMENT_TYPE_SPACER) {
        e->measured_size = static_cast<SpacerElement*>(e)->size;

    // @measure_expanded
    } else if (e->type == ELEMENT_TYPE_EXPANDED) {
        ExpandedElement* expanded = static_cast<ExpandedElement*>(e);
        Vec2 max_content_size = {};
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = MeasureElement(element_index, available_size);
            max_content_size = Max(max_content_size, child->measured_size);
        }

        e->measured_size = max_content_size;
        e->measured_size[expanded->axis] = available_size[expanded->axis];

    // @measure_transformed
    } else if (e->type == ELEMENT_TYPE_TRANSFORM) {
        Vec2 max_content_size = {};
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = MeasureElement(element_index, available_size);
            max_content_size = Max(max_content_size, child->measured_size);
        }

        e->measured_size = max_content_size;

    // @measure_textbox
    } else if (e->type == ELEMENT_TYPE_TEXTBOX) {
        e->measured_size = available_size;

    // @measure_grid
    } else if (e->type == ELEMENT_TYPE_GRID) {
        element_index = MeasureGrid(element_index - 1, available_size);

    // @measure_scrollable
    } else if (e->type == ELEMENT_TYPE_SCROLLABLE) {
        ScrollableElement* scrollable = static_cast<ScrollableElement*>(e);
        float content_height = 0;
        float max_width = 0.0f;
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = MeasureElement(element_index, available_size);
            content_height += child->measured_size.y;
            max_width = Max(max_width, child->measured_size.x);
        }
        scrollable->content_height = content_height;
        e->measured_size = available_size;
        e->measured_size.x = max_width;

    // @measure_popup
    } else if (e->type == ELEMENT_TYPE_POPUP) {

        Vec2 max_content_size = VEC2_ZERO;
        Vec2 content_size = g_ui.ortho_size;
        for (u16 i = 0; i < e->child_count; i++) {
            Element* child = g_ui.elements[element_index];
            element_index = MeasureElement(element_index, content_size);
            max_content_size = Max(max_content_size, child->measured_size);
        }

        e->measured_size = max_content_size;

    } else {
        assert(false && "Unhandled element type in MeasureElements");
    }

    return element_index;
}

static int LayoutElement(int element_index, const Vec2& size) {
    Element* e = g_ui.elements[element_index++];
    assert(e);

    e->rect = noz::Rect{0, 0, e->measured_size.x, e->measured_size.y};

    // @layout_container
    if (IsContainerType(e->type)) {
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

        if (e->type == ELEMENT_TYPE_CONTAINER) {
            for (u16 i = 0; i < e->child_count; i++) {
                Element* child = g_ui.elements[element_index];
                element_index = LayoutElement(element_index, content_size);
                child->rect.x += child_offset.x;
                child->rect.y += child_offset.y;
            }
        } else if (e->type == ELEMENT_TYPE_COLUMN) {
            for (u16 i = 0; i < e->child_count; i++) {
                Element* child = g_ui.elements[element_index];
                content_size.y = child->measured_size.y;
                element_index = LayoutElement(element_index, content_size);
                child->rect.y += child_offset.y;
                child->rect.x += child_offset.x;
                child_offset.y += GetSize(child->rect).y + container->style.spacing;
            }
        } else if (e->type == ELEMENT_TYPE_ROW) {
            for (u16 i = 0; i < e->child_count; i++) {
                Element* child = g_ui.elements[element_index];
                content_size.x = child->measured_size.x;
                element_index = LayoutElement(element_index, content_size);
                child->rect.x += child_offset.x;
                child->rect.y += child_offset.y;
                child_offset.x += GetSize(child->rect).x + container->style.spacing;
            }
        }

        e->rect.x += container->style.margin.left;
        e->rect.y += container->style.margin.top;

        if (subtract_margin_x)
            e->rect.width -= container->style.margin.left + container->style.margin.right;

        if (subtract_margin_y)
            e->rect.height -= container->style.margin.top + container->style.margin.bottom;

    // @layout_expanded
    } else if (e->type == ELEMENT_TYPE_EXPANDED) {
        Vec2 content_size = size; //  GetSize(e->rect);
        for (u16 i = 0; i < e->child_count; i++)
            element_index = LayoutElement(element_index, content_size);

    } else if (e->type == ELEMENT_TYPE_SCENE) {
        LabelElement* label = static_cast<LabelElement*>(e);
        label->rect.width = size.x;
        label->rect.height = size.y;

    // @layout_label
    } else if (e->type == ELEMENT_TYPE_LABEL) {
        LabelElement* label = static_cast<LabelElement*>(e);
        label->rect.width = size.x;
        label->rect.height = size.y;

    // @layout_image
    } else if (e->type == ELEMENT_TYPE_IMAGE) {
        ImageElement* image = static_cast<ImageElement*>(e);
        image->rect.width = size.x;
        image->rect.height = size.y;

    } else if (e->type == ELEMENT_TYPE_CANVAS) {
        CanvasElement* canvas = static_cast<CanvasElement*>(e);
        canvas->rect.x = canvas->position.x;
        canvas->rect.y = canvas->position.y;

        Vec2 content_size = GetSize(e->rect);
        for (u16 i = 0; i < e->child_count; i++)
            element_index = LayoutElement(element_index, content_size);
    } else if (e->type == ELEMENT_TYPE_SPACER) {
    } else if (e->type == ELEMENT_TYPE_TRANSFORM) {
        e->rect.width = size.x;
        e->rect.height = size.y;
        Vec2 content_size = GetSize(e->rect);
        for (u16 i = 0; i < e->child_count; i++)
            element_index = LayoutElement(element_index, content_size);

    // @layout_textbox
    } else if (e->type == ELEMENT_TYPE_TEXTBOX) {
        e->rect.width = size.x;
        e->rect.height = size.y;

    // @layout_grid
    } else if (e->type == ELEMENT_TYPE_GRID) {
        e->rect.width = size.x;
        e->rect.height = size.y;
        element_index = LayoutGrid(element_index - 1, GetSize(e->rect));

    // @layout_scrollable
    } else if (e->type == ELEMENT_TYPE_SCROLLABLE) {
        e->rect.width = size.x;
        e->rect.height = size.y;
        Vec2 content_size = GetSize(e->rect);
        for (u16 i = 0; i < e->child_count; i++)
            element_index = LayoutElement(element_index, content_size);

    // @layout_popup
    } else if (e->type == ELEMENT_TYPE_POPUP) {
        Vec2 content_size = GetSize(e->rect);
        for (u16 i = 0; i < e->child_count; i++)
            element_index = LayoutElement(element_index, content_size);

        PopupElement* popup = static_cast<PopupElement*>(e);
        Vec2 parent_size = size;
        const AlignInfo& anchor_info = g_align_info[popup->style.anchor];
        const AlignInfo& align_info = g_align_info[popup->style.align];

        // Anchor point on parent rect
        float anchor_x = anchor_info.has_x ? parent_size.x * anchor_info.x : 0.0f;
        float anchor_y = anchor_info.has_y ? parent_size.y * anchor_info.y : 0.0f;

        // Which point of the popup aligns to the anchor
        float popup_offset_x = align_info.has_x ? e->rect.width * align_info.x : 0.0f;
        float popup_offset_y = align_info.has_y ? e->rect.height * align_info.y : 0.0f;

        e->rect.x = anchor_x - popup_offset_x + popup->style.margin.left;
        e->rect.y = anchor_y - popup_offset_y + popup->style.margin.top;

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

    // For scrollable elements, apply scroll offset to children
    if (e->type == ELEMENT_TYPE_SCROLLABLE) {
        ScrollableElement* scrollable = static_cast<ScrollableElement*>(e);
        Mat3 scroll_transform = e->local_to_world * Translate({0, -scrollable->offset});
        for (u32 i = 0; i < e->child_count; i++)
            element_index = CalculateTransforms(element_index, scroll_transform);
    } else {
        for (u32 i = 0; i < e->child_count; i++)
            element_index = CalculateTransforms(element_index, e->local_to_world);
    }

    return element_index;
}

static void DrawCanvas(CanvasElement* canvas, const Mat3& transform){
    Update(g_ui.camera);
    BindCamera(g_ui.camera);

    if (canvas->style.color.a > F32_EPSILON) {
        BindTransform(transform * Scale(Vec2{canvas->rect.width, canvas->rect.height}));
        BindMaterial(g_ui.element_material);
        BindColor(canvas->style.color, canvas->style.color_offset);
        DrawMesh(g_ui.element_mesh);
    }
}

static void DrawContainer(ContainerElement* container, const Mat3& transform) {
    bool has_border = container->style.border.radius > 0.0f ||
        (container->style.border.width > 0.0f && container->style.border.color.a > 0.0f);
    if (container->style.color.a <= 0.0f && !has_border)
        return;

    BindTransform(transform * Scale(Vec2{container->rect.width, container->rect.height}));
    BindMaterial(g_ui.element_material);
    BindColor(container->style.color,container->style.color_offset);

    struct BorderVertex {
        float radius;
    };
    struct BorderFragment {
        Color color;
        float border_ratio;
        float square_corners;
        float padding0;
        float padding1;
    };

    // When border_radius is 0 but we have a visible border, use square corners
    // and set the vertex radius to border_width so the mesh expands properly
    bool use_square_corners = container->style.border.radius == 0.0f &&
        container->style.border.width > 0.0f &&
        container->style.border.color.a > 0.0f;

    float vertex_radius = use_square_corners
        ? container->style.border.width
        : container->style.border.radius;

    // Precompute border_ratio (border_width / vertex_radius)
    float border_ratio = vertex_radius > 0.0f
        ? container->style.border.width / vertex_radius
        : 0.0f;

    BorderVertex v_border = {
        .radius = vertex_radius
    };
    BindVertexUserData(&v_border, sizeof(BorderVertex));

    BorderFragment f_border = {
        .color = container->style.border.color,
        .border_ratio = border_ratio,
        .square_corners = use_square_corners ? 1.0f : 0.0f
    };
    BindFragmentUserData(&f_border, sizeof(BorderFragment));

    if (has_border) {
        DrawMesh(g_ui.element_with_border_mesh);
    } else {
        DrawMesh(g_ui.element_mesh);
    }
}

static int DrawElement(int element_index, bool is_popup) {
    Element* e = g_ui.elements[element_index++];
    const Mat3& transform = e->local_to_world;

    // @render_label
    if (e->type == ELEMENT_TYPE_LABEL) {
        LabelElement* l = static_cast<LabelElement*>(e);
        Mesh* mesh = l->cached_mesh ? GetMesh(l->cached_mesh->text_mesh) : nullptr;
        if (mesh) {
            const AlignInfo& align = g_align_info[l->style.align];
            Vec2 text_size = GetSize(l->cached_mesh->text_mesh);
            Vec2 text_offset = VEC2_ZERO;
            if (align.has_x) text_offset.x = (e->rect.width - text_size.x) * align.x;
            if (align.has_y) text_offset.y = (e->rect.height - text_size.y) * align.y;

            // Snap text position and size to integer pixel boundaries for crisp rendering
            Vec2 ui_pos = TransformPoint(e->local_to_world, text_offset);
            Vec2 ui_end = TransformPoint(e->local_to_world, text_offset + text_size);

            Vec2 screen_pos = WorldToScreen(g_ui.camera, ui_pos);
            Vec2 screen_end = WorldToScreen(g_ui.camera, ui_end);
            Vec2 screen_size = screen_end - screen_pos;

            // Snap position, then size, then calculate end from snapped values
            screen_pos.x = Floor(screen_pos.x);
            screen_pos.y = Floor(screen_pos.y);
            screen_size.x = Floor(screen_size.x);
            screen_size.y = Ceil(screen_size.y) + 1;
            screen_end = screen_pos + screen_size;

            Vec2 snapped_ui_pos = ScreenToWorld(g_ui.camera, screen_pos);
            Vec2 snapped_ui_end = ScreenToWorld(g_ui.camera, screen_end);
            Vec2 snapped_ui_size = snapped_ui_end - snapped_ui_pos;

            Vec2 scale = snapped_ui_size / text_size;

            BindTransform(Translate(snapped_ui_pos) * Scale(scale));
            BindColor(l->style.color);
            BindMaterial(l->style.material ? l->style.material : GetMaterial(l->cached_mesh->text_mesh));
            DrawMesh(mesh);
        }

    // @render_image
    } else if (e->type == ELEMENT_TYPE_IMAGE) {
        ImageElement* image = static_cast<ImageElement*>(e);
        Bounds2 image_bounds;
        Vec2 image_size;
        Vec2 image_scale = VEC2_ONE;
        if (image->mesh) {
            image_bounds = GetBounds(image->mesh);
            image_size = GetSize(image_bounds);
            BindMaterial(image->style.material);
        } else if (image->texture) {
            image_scale = ToVec2(GetSize(image->texture)) * Vec2{1,-1};
            image_size = ToVec2(GetSize(image->texture));
            image_bounds = GetBounds(g_ui.image_element_mesh);
            image_bounds.min *= image_size;
            image_bounds.max *= image_size;
            BindShader(SHADER_UI_IMAGE_TEXTURE);
            BindTexture(image->texture);
        } else {
            return element_index;
        }

        BindColor(image->style.color, image->style.color_offset);

        Mat3 image_transform;
        if (image->style.stretch == IMAGE_STRETCH_UNIFORM) {
            float scale_x = e->rect.width / image_size.x;
            float scale_y = e->rect.height / image_size.y;
            float uniform_scale = Min(scale_x, scale_y) * image->style.scale;

            const AlignInfo& align = g_align_info[image->style.align];
            Vec2 image_offset = VEC2_ZERO;
            if (align.has_x)
                image_offset.x = (e->rect.width - image_size.x * uniform_scale) * align.x;
            if (align.has_y)
                image_offset.y = (e->rect.height - image_size.y * uniform_scale) * align.y;

            image_transform = transform *
                Translate(Vec2{-image_bounds.min.x, -image_bounds.min.y} * uniform_scale + image_offset) *
                Scale(image_scale * uniform_scale);
        } else if (image->style.stretch == IMAGE_STRETCH_FILL) {
            image_scale = Vec2 {e->rect.width / image_size.x, e->rect.height / image_size.y} * image_scale;
            Vec2 image_offset = Vec2{
                -image_bounds.min.x * image_scale.x,
                -image_bounds.min.y * image_scale.y
            };
            image_transform = transform * Translate(image_offset) * Scale(image_scale);
        } else {
            // render the image at the align point
            const AlignInfo& align = g_align_info[image->style.align];
            Vec2 image_offset = VEC2_ZERO;
            if (align.has_x) image_offset.x = e->rect.width * align.x;
            if (align.has_y) image_offset.y = e->rect.height * align.y;
            image_transform = transform * Translate(image_offset) * Scale(image_scale * image->style.scale);
        }

        if (image->mesh) {
            if (image->animated_time != 0.0f)
                DrawMesh(image->mesh, image_transform, image->animated_time);
            else
                DrawMesh(image->mesh, image_transform);
        } else if (image->texture)
            DrawMesh(g_ui.image_element_mesh, image_transform);

    // @render_container
    } else if (IsContainerType(e->type)) {
        ContainerElement* container = static_cast<ContainerElement*>(e);

        // Draw container visual (background/border) FIRST, before stencil
        DrawContainer(container, transform);

        // Set up stencil clipping for children only
        if (container->style.clip) {
            BeginClip();

            // Draw container shape for stencil (writes to stencil, not color)
            BindTransform(transform * Scale(Vec2{container->rect.width, container->rect.height}));
            BindMaterial(g_ui.element_material);
            BindColor(COLOR_WHITE);  // Color doesn't matter, stencil only

            // Use the border mesh if we have a radius, otherwise simple quad
            struct BorderVertex { float radius; };
            struct BorderFragment { Color color; float border_ratio; float square_corners; float p0; float p1; };
            if (container->style.border.radius > 0.0f) {
                BorderVertex v = { .radius = container->style.border.radius };
                BorderFragment f = { .color = COLOR_TRANSPARENT, .border_ratio = 0.0f, .square_corners = 0.0f };
                BindVertexUserData(&v, sizeof(v));
                BindFragmentUserData(&f, sizeof(f));
                DrawMesh(g_ui.element_with_border_mesh);
            } else {
                BorderVertex v = { .radius = 0.0f };
                // Set border_ratio to -1 to signal "no clipping" in shader
                BorderFragment f = { .color = COLOR_TRANSPARENT, .border_ratio = -1.0f, .square_corners = 1.0f };
                BindVertexUserData(&v, sizeof(v));
                BindFragmentUserData(&f, sizeof(f));
                DrawMesh(g_ui.element_mesh);
            }

            EndClipWrite();
        }

    // @render_canvas
    } else if (e->type == ELEMENT_TYPE_CANVAS) {
        DrawCanvas(static_cast<CanvasElement*>(e), transform);

    // @render_scene
    } else if (e->type == ELEMENT_TYPE_SCENE) {
        SceneElement* scene = static_cast<SceneElement*>(e);
        if (scene->style.camera && scene->draw_scene) {
            Vec2 scene_size = GetWorldSize(scene->style.camera);
            float scale_x = e->rect.width / scene_size.x;
            float scale_y = e->rect.height / scene_size.y;
            float uniform_scale = Min(scale_x, scale_y);

            const AlignInfo& align = g_align_info[scene->style.align];
            Vec2 scene_offset = VEC2_ZERO;
            if (align.has_x)
                scene_offset.x = (e->rect.width - scene_size.x * uniform_scale) * align.x;
            if (align.has_y)
                scene_offset.y = (e->rect.height - scene_size.y * uniform_scale) * align.y;

            Vec2 viewport_size = scene_size * uniform_scale;
            Vec2 viewport_pos = Vec2{
                e->rect.x + scene_offset.x,
                e->rect.y + scene_offset.y
            };

            Vec2 viewport_tl = WorldToScreen(g_ui.camera, TransformPoint(transform, viewport_pos));
            Vec2 viewport_br = WorldToScreen(g_ui.camera, TransformPoint(transform, viewport_pos + viewport_size));

            SetViewport(scene->style.camera, {
                viewport_tl.x,
                viewport_tl.y,
                viewport_br.x - viewport_tl.x,
                viewport_br.y - viewport_tl.y
            });

            BindCamera(scene->style.camera);
            scene->draw_scene(scene->style.user_data);

            // Restore UI camera
            Update(g_ui.camera);
            BindCamera(g_ui.camera);
            BindDepth(g_ui.depth, 0);
        }
    } else if (e->type == ELEMENT_TYPE_ROW) {
        // BindTransform(e->local_to_world * Scale({e->rect.width, e->rect.height}));
        // BindColor(COLOR_BLUE);
        // BindMaterial(g_ui.element_material);
        // DrawMesh(g_ui.element_mesh);

    // @render_scrollable
    } else if (e->type == ELEMENT_TYPE_SCROLLABLE) {
        // Set up stencil clipping for scrollable content
        BeginClip();

        // Draw scrollable area shape for stencil (writes to stencil, not color)
        BindTransform(transform * Scale(Vec2{e->rect.width, e->rect.height}));
        BindMaterial(g_ui.element_material);
        BindColor(COLOR_WHITE);  // Color doesn't matter, stencil only
        struct BorderVertex { float radius; };
        struct BorderFragment { Color color; float border_ratio; float square_corners; float p0; float p1; };
        BorderVertex v = { .radius = 0.0f };
        BorderFragment f = { .color = COLOR_TRANSPARENT, .border_ratio = -1.0f, .square_corners = 1.0f };
        BindVertexUserData(&v, sizeof(v));
        BindFragmentUserData(&f, sizeof(f));
        DrawMesh(g_ui.element_mesh);

        EndClipWrite();

        // @render_popup
    } else if (e->type == ELEMENT_TYPE_POPUP && !is_popup) {
        return e->next_sibling_index;
    }

    // Draw children
    for (u32 i = 0; i < e->child_count; i++)
        element_index = DrawElement(element_index, false);

    // End clipping after children are drawn
    if (IsContainerType(e->type)) {
        ContainerElement* container = static_cast<ContainerElement*>(e);
        if (container->style.clip) {
            EndClip();
        }
    }

    if (e->type == ELEMENT_TYPE_SCROLLABLE) {
        EndClip();
    }

    return element_index;
}

void BeginUI(u32 ref_width, u32 ref_height) {
    g_ui.ref_size = { static_cast<i32>(ref_width), static_cast<i32>(ref_height) };
    g_ui.element_stack_count = 0;
    g_ui.element_count = 0;
    g_ui.popup_count = 0;

    Clear(g_ui.allocator);

    Vec2Int screen_size = GetScreenSize();

    f32 rw = static_cast<f32>(g_ui.ref_size.x);
    f32 rh = static_cast<f32>(g_ui.ref_size.y);
    f32 sw = static_cast<f32>(screen_size.x);
    f32 sh = static_cast<f32>(screen_size.y);

    if (rw > 0 && rh > 0) {
        f32 aspect_ref = rw / rh;
        f32 aspect_screen = sw / sh;

        if (aspect_screen >= aspect_ref) {
            g_ui.ortho_size.y = rh;
            g_ui.ortho_size.x = rh * aspect_screen;
        } else {
            g_ui.ortho_size.x = rw;
            g_ui.ortho_size.y = rw / aspect_screen;
        }
    } else if (rw > 0) {
        g_ui.ortho_size.x = rw;
        g_ui.ortho_size.y = rw * (sh / sw);
    } else if (rh > 0) {
        g_ui.ortho_size.y = rh;
        g_ui.ortho_size.x = rh * (sw / sh);
    } else {
        g_ui.ortho_size.x = sw;
        g_ui.ortho_size.y = sh;
    }

    SetExtents(g_ui.camera, 0, g_ui.ortho_size.x, 0, g_ui.ortho_size.y);

    UpdateInputState(g_ui.input);

    SetFocus(g_ui.pending_focus_id);
}

inline u8 WrapElementId(u8 id, int offset) {
    int new_id = static_cast<int>(id) + offset;
    if (new_id < ELEMENT_ID_MIN)
        new_id = ELEMENT_ID_MAX;
    else if (new_id >= ELEMENT_ID_MAX)
        new_id = ELEMENT_ID_MIN;
    return static_cast<u8>(new_id);
}

static void HandleInput() {
    Vec2 mouse = ScreenToWorld(g_ui.camera, GetMousePosition());
    bool mouse_left_pressed = WasButtonPressed(g_ui.input, MOUSE_LEFT);
    bool button_down = IsButtonDown(g_ui.input, MOUSE_LEFT);
    bool focus_element_pressed = false;
    bool focus_element_found = false;
    TextboxElement* focused_textbox = nullptr;
    noz::Rect focused_textbox_rect = {};

    // Close popups when clicking outside
    g_ui.close_popups = false;
    if (mouse_left_pressed && g_ui.popup_count > 0) {
        bool click_inside_popup = false;
        for (int i = 0; i < g_ui.popup_count; i++) {
            PopupElement* popup = static_cast<PopupElement*>(g_ui.popups[i]);
            Vec2 local_mouse = TransformPoint(popup->world_to_local, mouse);
            if (Contains(Bounds2{0, 0, popup->rect.width, popup->rect.height}, local_mouse)) {
                click_inside_popup = true;
                break;
            }
        }
        if (!click_inside_popup) {
            g_ui.close_popups = true;
            return;
        }
    }

    // First pass: set flags for all elements (don't consume yet)
    for (u16 element_index=g_ui.element_count; element_index>0; element_index--) {
        Element* e = g_ui.elements[element_index-1];
        if (e->id == ELEMENT_ID_NONE) continue;

        ElementState& state = g_ui.element_states[e->id];
        state.rect = e->rect;  // Store rect for GetElementRect
        Vec2 local_mouse = TransformPoint(e->world_to_local, mouse);
        bool mouse_over = Contains(Bounds2{0,0,e->rect.width, e->rect.height}, local_mouse);

        focus_element_found &= e->id == g_ui.focus_id;

        if (g_ui.focus_id == e->id && e->type == ELEMENT_TYPE_TEXTBOX) {
            focused_textbox = static_cast<TextboxElement*>(e);
            Vec2 tl = WorldToScreen(g_ui.camera, TransformPoint(focused_textbox->local_to_world));
            Vec2 br = WorldToScreen(g_ui.camera, TransformPoint(focused_textbox->local_to_world, Vec2{focused_textbox->rect.width, focused_textbox->rect.height}));
            focused_textbox_rect = noz::Rect{tl.x, tl.y, br.x - tl.x, br.y - tl.y};
        }

        if (mouse_over)
            state.flags = state.flags | ELEMENT_FLAG_HOVERED;
        else
            state.flags = state.flags & ~ELEMENT_FLAG_HOVERED;

        if (mouse_over && mouse_left_pressed) {
            state.flags = state.flags | ELEMENT_FLAG_PRESSED;
            if (!focus_element_pressed && e->id != 0) {
                focus_element_pressed = true;
                focus_element_found = true;
                SetPendingFocus(e->id);
            }
        } else {
            state.flags = state.flags & ~ELEMENT_FLAG_PRESSED;
        }

        if (mouse_over && button_down) {
            state.flags = state.flags | ELEMENT_FLAG_DOWN;
        } else {
            state.flags = state.flags & ~ELEMENT_FLAG_DOWN;
        }
    }

    // Handle scrollable element drag
    if (!button_down) {
        // Mouse released, stop scrolling
        g_ui.active_scroll_id = ELEMENT_ID_NONE;
    } else if (g_ui.active_scroll_id != ELEMENT_ID_NONE) {
        // Continue scrolling active element
        float delta_y = g_ui.last_scroll_mouse_y - mouse.y;
        g_ui.last_scroll_mouse_y = mouse.y;

        // Find the scrollable element and update its offset
        for (u16 i = 0; i < g_ui.element_count; i++) {
            Element* e = g_ui.elements[i];
            if (e->type == ELEMENT_TYPE_SCROLLABLE && e->id == g_ui.active_scroll_id) {
                    ScrollableElement* scrollable = static_cast<ScrollableElement*>(e);
                ElementState& state = g_ui.element_states[e->id];

                // Update scroll offset
                float new_offset = scrollable->offset + delta_y;
                float max_scroll = Max(0.0f, scrollable->content_height - e->rect.height);
                new_offset = Clamp(new_offset, 0.0f, max_scroll);

                scrollable->offset = new_offset;
                state.scroll_offset = new_offset;
                break;
            }
        }
    } else if (mouse_left_pressed) {
        // Start scrolling if pressed on a scrollable element
        for (u16 i = g_ui.element_count; i > 0; i--) {
            Element* e = g_ui.elements[i - 1];
            if (e->type == ELEMENT_TYPE_SCROLLABLE && e->id != ELEMENT_ID_NONE) {
                ElementState& state = g_ui.element_states[e->id];
                if (state.flags & ELEMENT_FLAG_PRESSED) {
                    g_ui.active_scroll_id = e->id;
                    g_ui.last_scroll_mouse_y = mouse.y;
                    break;
                }
            }
        }
    }

    if (focused_textbox) {
        Text& text = g_ui.element_states[focused_textbox->id].text;
        int font_size = static_cast<int>(focused_textbox->style.font_size * GetUIScale());
        if (g_ui.textbox_id != focused_textbox->id) {
            g_ui.textbox_id = focused_textbox->id;
            PlatformShowTextbox(focused_textbox_rect, text, {
                focused_textbox->style.background_color,
                focused_textbox->style.text_color,
                font_size,
                focused_textbox->style.password
            });
        } else {
            PlatformUpdateTextboxRect(focused_textbox_rect, font_size);
        }
    } else if (g_ui.textbox_id) {
        PlatformHideTextbox();
        g_ui.textbox_id = 0;
    }

    // Second pass: consume button for topmost pressed element
    for (u32 i=g_ui.element_count; i>0; i--) {
        Element* e = g_ui.elements[i-1];
        if (e->id == ELEMENT_ID_NONE) continue;
        ElementState& prev_state = g_ui.element_states[e->id];
        if ((prev_state.flags & ELEMENT_FLAG_PRESSED) && e->type != ELEMENT_TYPE_CANVAS) {
            ConsumeButton(MOUSE_LEFT);
            break;  // Only consume once for the topmost element
        }
    }

    // Cycle through ui elements
    if (WasButtonPressed(KEY_TAB)) {
        int focus_offset = IsShiftDown() ? -1 : 1;
        ElementId focus_id = g_ui.focus_id;

        if (focus_id != 0) {
            u16 focus_index = g_ui.element_states[focus_id].index;
            if (focus_index > 0 && focus_index < g_ui.element_count) {
                Element* f = g_ui.elements[focus_index];
                if (f->type == ELEMENT_TYPE_CONTAINER) {
                    ContainerElement* container = static_cast<ContainerElement*>(f);
                    if (IsShiftDown() && container->style.nav.prev != 0)
                        focus_id = container->style.nav.prev;
                    else if (!IsShiftDown() && container->style.nav.next != 0)
                        focus_id = container->style.nav.next;
                }
            } else {
                focus_id = ELEMENT_ID_NONE;
            }
        }

        // Helper to check if element is focusable (valid index and in focus canvas)
        auto is_focusable = [](ElementId id) {
            const ElementState& state = g_ui.element_states[id];
            if (state.index == 0) return false;
            // If no canvas has focus (current_focus_canvas_id == 0), allow all elements
            // Otherwise, only allow elements in the focus canvas
            if (g_ui.current_focus_canvas_id == 0) return true;
            return state.canvas_id == g_ui.current_focus_canvas_id;
        };

        if (focus_id == ELEMENT_ID_NONE) {
            for (focus_id = ELEMENT_ID_MIN;
                 !is_focusable(focus_id) && focus_id < ELEMENT_ID_MAX;
                 focus_id++);
        } else if (focus_id == g_ui.focus_id) {
            for (focus_id = WrapElementId(focus_id, focus_offset);
                 !is_focusable(focus_id) && focus_id != g_ui.focus_id;
                 focus_id = WrapElementId(focus_id, focus_offset));
        }

        SetPendingFocus(focus_id);
    }

    // Clear focus when clicking outside of focused element and a textbox is active
    if (mouse_left_pressed && !focus_element_pressed && g_ui.textbox_id != 0)
        SetPendingFocus(ELEMENT_ID_NONE);
    // Native textbox lost focus?
    else if (g_ui.textbox_id != ELEMENT_ID_NONE && g_ui.focus_id == g_ui.textbox_id && !PlatformIsTextboxVisible())
        SetPendingFocus(ELEMENT_ID_NONE);
}

void EndUI() {
    UpdateDebugUI();

    for (int element_index=0; element_index < g_ui.element_count; )
        element_index = MeasureElement(element_index, g_ui.ortho_size);
    for (int element_index=0; element_index < g_ui.element_count; )
        element_index = LayoutElement(element_index, g_ui.ortho_size);
    for (int element_index=0; element_index < g_ui.element_count; )
        element_index = CalculateTransforms(element_index, MAT3_IDENTITY);

    HandleInput();
}

void DrawUI() {
    BindDepth(g_ui.depth, 0);
    for (u32 element_index = 0; element_index < g_ui.element_count; )
        element_index = DrawElement(element_index, false);

    for (int popup_index=0; popup_index < g_ui.popup_count; popup_index++) {
        PopupElement* p = static_cast<PopupElement*>(g_ui.popups[popup_index]);
        for (u32 element_index = p->index; element_index < p->next_sibling_index; )
            element_index = DrawElement(element_index, true);
    }

    // cursor
    BindDepth(g_ui.depth, 0);
    if (GetApplicationTraits()->draw_cursor) {
        Vec2 screen_size = ToVec2(GetScreenSize());
        Vec2 cursor_pos = GetMousePosition();
        SetExtents(g_ui.cursor_camera, 0, screen_size.x, 0, screen_size.y);
        BindCamera(g_ui.cursor_camera);
        GetApplicationTraits()->draw_cursor(Translate(cursor_pos));
    }
}

void SetUIPaletteTexture(Texture* texture) {
    if (!texture) return;
    SetTexture(g_ui.image_element_material, texture);
}

static void CreateElementWithBorderMesh() {
    PushScratch();
    MeshBuilder* mb = CreateMeshBuilder(ALLOCATOR_SCRATCH, 1024, 1024);
    // 0 - 3
    AddVertex(mb, MeshVertex{.position=Vec2{0,0}, .uv=Vec2{ 1, 1}, .normal=Vec2{ 0, 0}});
    AddVertex(mb, MeshVertex{.position=Vec2{0,0}, .uv=Vec2{ 0, 1}, .normal=Vec2{ 1, 0}});
    AddVertex(mb, MeshVertex{.position=Vec2{1,0}, .uv=Vec2{ 0, 1}, .normal=Vec2{-1, 0}});
    AddVertex(mb, MeshVertex{.position=Vec2{1,0}, .uv=Vec2{ 1, 1}, .normal=Vec2{ 0, 0}});

    // 4 - 7
    AddVertex(mb, MeshVertex{.position=Vec2{0,0}, .uv=Vec2{ 1, 0}, .normal=Vec2{ 0, 1}});
    AddVertex(mb, MeshVertex{.position=Vec2{0,0}, .uv=Vec2{ 0, 0}, .normal=Vec2{ 1, 1}});
    AddVertex(mb, MeshVertex{.position=Vec2{1,0}, .uv=Vec2{ 0, 0}, .normal=Vec2{-1, 1}});
    AddVertex(mb, MeshVertex{.position=Vec2{1,0}, .uv=Vec2{ 1, 0}, .normal=Vec2{ 0, 1}});

    // 8 - 11
    AddVertex(mb, MeshVertex{.position=Vec2{0,1}, .uv=Vec2{ 1, 0}, .normal=Vec2{ 0,-1}});
    AddVertex(mb, MeshVertex{.position=Vec2{0,1}, .uv=Vec2{ 0, 0}, .normal=Vec2{ 1,-1}});
    AddVertex(mb, MeshVertex{.position=Vec2{1,1}, .uv=Vec2{ 0, 0}, .normal=Vec2{-1,-1}});
    AddVertex(mb, MeshVertex{.position=Vec2{1,1}, .uv=Vec2{ 1, 0}, .normal=Vec2{ 0,-1}});

    // 12 - 15
    AddVertex(mb, MeshVertex{.position=Vec2{0,1}, .uv=Vec2{ 1, 1}, .normal=Vec2{ 0, 0}});
    AddVertex(mb, MeshVertex{.position=Vec2{0,1}, .uv=Vec2{ 0, 1}, .normal=Vec2{ 1, 0}});
    AddVertex(mb, MeshVertex{.position=Vec2{1,1}, .uv=Vec2{ 0, 1}, .normal=Vec2{-1, 0}});
    AddVertex(mb, MeshVertex{.position=Vec2{1,1}, .uv=Vec2{ 1, 1}, .normal=Vec2{ 0, 0}});

    // top
    AddTriangle(mb, 0, 1, 4);
    AddTriangle(mb, 4, 1, 5);
    AddTriangle(mb, 1, 2, 5);
    AddTriangle(mb, 5, 2, 6);
    AddTriangle(mb, 2, 3, 6);
    AddTriangle(mb, 6, 3, 7);

    // middle
    AddTriangle(mb, 4, 5, 8);
    AddTriangle(mb, 8, 5, 9);
    AddTriangle(mb, 9, 5, 6);
    AddTriangle(mb, 9, 6, 10);
    AddTriangle(mb, 6, 7, 10);
    AddTriangle(mb, 10, 7, 11);

    // bottom
    AddTriangle(mb, 8, 9, 12);
    AddTriangle(mb, 12, 9, 13);
    AddTriangle(mb, 9, 10, 13);
    AddTriangle(mb, 13, 10, 14);
    AddTriangle(mb, 10, 11, 14);
    AddTriangle(mb, 14, 11, 15);

    g_ui.element_with_border_mesh = CreateMesh(ALLOCATOR_DEFAULT, mb, NAME_NONE);

    PopScratch();
}

static void CreateElementMesh() {
    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(builder, MeshVertex{.position={0.0f,0.0f}, .uv={0.0f,0.0f}});
    AddVertex(builder, MeshVertex{.position={1.0f,0.0f}, .uv={0.0f,0.0f}});
    AddVertex(builder, MeshVertex{.position={1.0f,1.0f}, .uv={0.0f,0.0f}});
    AddVertex(builder, MeshVertex{.position={0.0f,1.0f}, .uv={0.0f,0.0f}});
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);

    g_ui.element_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, GetName("element"));
    PopScratch();
}

static void CreateImageElementMesh() {
    PushScratch();
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(builder, Vec2{ -0.5f, -0.5f }, { 0.0f, 1.0f });
    AddVertex(builder, Vec2{  0.5f, -0.5f }, { 1.0f, 1.0f });
    AddVertex(builder, Vec2{  0.5f,  0.5f }, { 1.0f, 0.0f });
    AddVertex(builder, Vec2{ -0.5f,  0.5f }, { 0.0f, 0.0f });
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);
    g_ui.image_element_mesh = CreateMesh(ALLOCATOR_DEFAULT, builder, GetName("image"));
    PopScratch();
}

bool IsClosed() {
    return GetCurrentElement()->type == ELEMENT_TYPE_POPUP && g_ui.close_popups;
}

void InitUI(const ApplicationTraits* traits) {
    memset(&g_ui, 0, sizeof(g_ui));
    g_ui.allocator = CreateArenaAllocator(sizeof(FatElement) * MAX_ELEMENTS, "UI");
    g_ui.camera = CreateCamera(ALLOCATOR_DEFAULT);
    g_ui.element_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);
    g_ui.image_element_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI_IMAGE);
    g_ui.input = CreateInputSet(ALLOCATOR_DEFAULT);
    g_ui.text_mesh_allocator = CreatePoolAllocator(sizeof(CachedTextMesh), MAX_TEXT_MESHES);
    g_ui.depth = traits->ui_depth >= F32_MAX ? traits->renderer.max_depth - 0.01f : traits->ui_depth;
    g_ui.popup_count = 0;
    g_ui.close_popups = false;

    g_ui.cursor_camera = CreateCamera(ALLOCATOR_DEFAULT);

    EnableButton(g_ui.input, MOUSE_LEFT);
    if (TEXTURE)
        SetUIPaletteTexture(TEXTURE[0]);

    CreateElementMesh();
    CreateElementWithBorderMesh();
    CreateImageElementMesh();

    for (int i=0; i<TEXT_MAX_LENGTH; i++)
        g_ui.password_mask.value[i] = '*';
    g_ui.password_mask.value[TEXT_MAX_LENGTH] = 0;
}

void ShutdownUI() {
    Destroy(g_ui.allocator);
    memset(&g_ui, 0, sizeof(g_ui));
}

//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "../platform.h"
constexpr int MAX_ELEMENTS = 1024;
constexpr int STYLE_STACK_SIZE = 16;

extern void UpdateInputState(InputSet* input_set);

enum ElementType : u16
{
    ELEMENT_TYPE_UNKNOWN = 0,
    ELEMENT_TYPE_NONE,
    ELEMENT_TYPE_CANVAS,
    ELEMENT_TYPE_LABEL,
    ELEMENT_TYPE_IMAGE,
    ELEMENT_TYPE_MESH
};

typedef u16 ElementFlags;
constexpr ElementFlags ELEMENT_FLAG_HOVER = 1 << 0;
constexpr ElementFlags ELEMENT_FLAG_MOUSE_ENTER = 1 << 1;
constexpr ElementFlags ELEMENT_FLAG_MOUSE_LEAVE = 1 << 2;

StyleSheet** STYLESHEET = nullptr;

struct CachedTextMesh
{
    TextMesh* text_mesh;
    int last_frame;
};

struct Element
{
    ElementType type;
    ElementFlags flags;
    Rect bounds;
    u32 parent;
    Style style;
    Vec2 measured_size;
    u32 index;
    u32 child_count;
    void* resource;
    Material* material;
    void* input_user_data;
    ElementInputFunc input_func;
    u64 hash;
};

struct UI
{
    Camera* camera;
    Mesh* element_quad;
    Material* element_material;
    Material* vignette_material;
    CachedTextMesh text_mesh_cache_values[1024];
    u64 text_mesh_cache_keys[1024];
    Map text_mesh_cache;
    Element elements[1024];
    u32 element_count;
    u32 element_stack[MAX_ELEMENTS];
    u32 element_stack_count;
    StyleSheet* style_sheet_stack[STYLE_STACK_SIZE];
    u32 style_stack_count;
    Vec2Int ref_size;
    Vec2 ortho_size;
    bool in_frame;
    InputSet* input;
    StyleSheet* default_style_sheet;
};

static UI g_ui = {};

u32 Layout(u32 element_index, Rect parent_bounds);

inline StyleSheet* GetCurrentStyleSheet()
{
    return g_ui.style_stack_count > 0 ? g_ui.style_sheet_stack[g_ui.style_stack_count-1] : nullptr;
}

static Mesh* CreateElementQuad(Allocator* allocator)
{
    MeshBuilder* builder = CreateMeshBuilder(ALLOCATOR_SCRATCH, 4, 6);
    AddVertex(builder, {0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 1.0f});
    AddVertex(builder, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f});
    AddVertex(builder, {1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f});
    AddVertex(builder, {0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f});
    AddTriangle(builder, 0, 1, 2);
    AddTriangle(builder, 0, 2, 3);
    auto mesh = CreateMesh(allocator, builder, GetName("element"));
    Free(builder);
    return mesh;
}

static float Evaluate(const StyleLength& length, float parent_value)
{
    switch (length.unit)
    {
    case STYLE_LENGTH_UNIT_AUTO:
        return parent_value;

    case STYLE_LENGTH_UNIT_FIXED:
        return length.value;

    case STYLE_LENGTH_UNIT_PERCENT:
        return parent_value * length.value;

    default:
        return 0.0f;
    }
}

static Element& GetCurrentElement()
{
    return g_ui.elements[g_ui.element_count-1];
}

void SetInputHandler(ElementInputFunc func, void* user_data)
{
    Element& e = GetCurrentElement();
    e.input_user_data = user_data;
    e.input_func = func;
}

static void BeginElement(ElementType type, const StyleId& style_id)
{
    assert(g_ui.in_frame);

    Element& e = g_ui.elements[g_ui.element_count++];
    e.type = type;
    e.parent = g_ui.element_stack_count > 0 ? g_ui.element_stack[g_ui.element_stack_count-1] : UINT32_MAX;
    e.style = GetStyle(style_id);
    e.index = g_ui.element_count-1;
    e.child_count = 0;
    g_ui.element_stack[g_ui.element_stack_count++] = e.index;

    u64 hash = Hash(e.index, e.parent, style_id.style_sheet_id);
    hash = Hash(type, hash, style_id.id);

    if (e.hash == hash)
    {
        Vec2 mouse = ScreenToWorld(g_ui.camera, GetMousePosition());
        if (e.type != ELEMENT_TYPE_CANVAS && Contains(e.bounds, mouse))
        {
            if ((e.flags & ELEMENT_FLAG_HOVER) == 0)
                e.flags |= ELEMENT_FLAG_MOUSE_ENTER;
            else
                e.flags &= ~ELEMENT_FLAG_MOUSE_ENTER;
            e.flags |= ELEMENT_FLAG_HOVER;
        }
        else
        {
            if (e.flags & ELEMENT_FLAG_HOVER)
                e.flags |= ELEMENT_FLAG_MOUSE_LEAVE;
            else
                e.flags &= ~ELEMENT_FLAG_MOUSE_LEAVE;
            e.flags &= ~ELEMENT_FLAG_HOVER;
        }
    }
    else
    {
        e.hash = hash;
        e.flags = 0;
        e.bounds = Rect(0,0,0,0);
    }

}

void BeginElement(const StyleId& style_id)
{
    BeginElement(ELEMENT_TYPE_NONE, style_id);
}


void EmptyElement(const StyleId& style_id)
{
    BeginElement(ELEMENT_TYPE_NONE, style_id);
    EndElement();
}

void EndElement()
{
    Element& e = g_ui.elements[g_ui.element_stack[g_ui.element_stack_count-1]];

    if (e.parent != UINT32_MAX)
    {
        Element& p = g_ui.elements[e.parent];
        p.child_count++;
    }
    g_ui.element_stack_count--;
}

void BeginWorldCanvas(Camera* camera, const Vec2& position, const Vec2& size, const StyleId& style_id)
{
    Vec2 flipped_size = { size.x, -size.y };
    Vec2 screen_pos = WorldToScreen(camera, {position.x, position.y});
    Vec2 screen_size = WorldToScreen(camera, flipped_size) - WorldToScreen(camera, VEC2_ZERO);
    screen_pos = screen_pos - screen_size * 0.5f;

    Vec2 ui_pos = ScreenToWorld(g_ui.camera, screen_pos);
    Vec2 ui_size = ScreenToWorld(g_ui.camera, screen_size) - ScreenToWorld(g_ui.camera, VEC2_ZERO);

    BeginElement(ELEMENT_TYPE_CANVAS, style_id);

    // Force the canvas element to fit the reference
    Element& e = GetCurrentElement();
    e.style.margin_left = { STYLE_KEYWORD_INLINE, STYLE_LENGTH_UNIT_FIXED, ui_pos.x };
    e.style.margin_top = { STYLE_KEYWORD_INLINE, STYLE_LENGTH_UNIT_FIXED, ui_pos.y };
    e.style.width = { STYLE_KEYWORD_INLINE, STYLE_LENGTH_UNIT_FIXED, ui_size.x };
    e.style.height = { STYLE_KEYWORD_INLINE, STYLE_LENGTH_UNIT_FIXED, ui_size.y };
}

void BeginCanvas(const StyleId& style_id)
{
    BeginElement(ELEMENT_TYPE_CANVAS, style_id);

    // Force the canvas element to fit the reference
    Element& e = GetCurrentElement();
    e.style.width = { STYLE_KEYWORD_INLINE, STYLE_LENGTH_UNIT_FIXED, g_ui.ortho_size.x };
    e.style.height = { STYLE_KEYWORD_INLINE, STYLE_LENGTH_UNIT_FIXED, g_ui.ortho_size.y };
}

void EndCanvas()
{
    EndElement();
}

void RenderCanvas(const Element& e)
{
    (void)e;
    UpdateCamera(g_ui.camera);
    BindCamera(g_ui.camera);
}

struct VignetteBuffer
{
    float intensity;
    float smoothness;
    float padding0;
    float padding1;
};

void RenderElement(const Element& e)
{
    if (e.type == ELEMENT_TYPE_CANVAS)
        RenderCanvas(e);

    if (e.style.background_color.value.a > 0)
    {
        BindTransform({e.bounds.x, e.bounds.y}, 0.0f, {e.bounds.width, e.bounds.height});
        BindMaterial(g_ui.element_material);
        BindColor(e.style.background_color.value);
        DrawMesh(g_ui.element_quad);
    }

    if (e.style.background_vignette_color.value.a > 0)
    {
        VignetteBuffer vignette = {
            .intensity = e.style.background_vignette_intensity.value,
            .smoothness = e.style.background_vignette_smoothness.value
        };

        BindTransform({e.bounds.x, e.bounds.y}, 0.0f, {e.bounds.width, e.bounds.height});
        BindMaterial(g_ui.vignette_material);
        BindColor(e.style.background_vignette_color.value);
        BindFragmentUserData(&vignette, sizeof(vignette));
        DrawMesh(g_ui.element_quad);
    }

    switch (e.type)
    {
    case ELEMENT_TYPE_LABEL:
    {
        if (!e.resource)
            break;

        Mesh* mesh = GetMesh((TextMesh*)e.resource);
        if (mesh)
        {
            Vec2 text_size = GetSize((TextMesh*)e.resource);
            
            // Calculate horizontal alignment offset
            float align_x = 0.0f;
            if (e.style.text_align.parameter.keyword != STYLE_KEYWORD_INHERIT)
            {
                switch (e.style.text_align.value)
                {
                case TEXT_ALIGN_MIN:
                    align_x = 0.0f;
                    break;
                case TEXT_ALIGN_CENTER:
                    align_x = (e.bounds.width - text_size.x) * 0.5f;
                    break;
                case TEXT_ALIGN_MAX:
                    align_x = e.bounds.width - text_size.x;
                    break;
                }
            }
            
            // Calculate vertical alignment offset
            float align_y = 0.0f;
            if (e.style.vertical_align.parameter.keyword != STYLE_KEYWORD_INHERIT)
            {
                switch (e.style.vertical_align.value)
                {
                case TEXT_ALIGN_MIN:
                    align_y = 0.0f;
                    break;
                case TEXT_ALIGN_CENTER:
                    align_y = (e.bounds.height - text_size.y) * 0.5f;
                    break;
                case TEXT_ALIGN_MAX:
                    align_y = e.bounds.height - text_size.y;
                    break;
                }
            }
            
            BindTransform({e.bounds.x + align_x, e.bounds.y + align_y}, 0.0f, {1,1});
            BindColor(e.style.color.value);
            BindMaterial(e.material);
            DrawMesh(mesh);
        }
        break;
    }

    case ELEMENT_TYPE_IMAGE:
    {
        Material* material = (Material*)e.resource;
        if (!material)
            material = g_ui.element_material;
        BindMaterial(material);
        BindColor(COLOR_WHITE);
        BindTransform({e.bounds.x, e.bounds.y}, 0.0f, {e.bounds.width, e.bounds.height});
        DrawMesh(g_ui.element_quad);
        break;
    }

    case ELEMENT_TYPE_MESH:
    {
        Bounds2 mesh_bounds = GetBounds((Mesh*)e.resource);
        BindMaterial(e.material);
        BindColor(e.style.color.value);
        Vec2 mesh_scale = Vec2 {
            e.bounds.width / (mesh_bounds.max.x - mesh_bounds.min.x),
            -e.bounds.height / (mesh_bounds.max.y - mesh_bounds.min.y)
        };
        BindTransform(
            Vec2{e.bounds.x + -mesh_bounds.min.x * mesh_scale.x, e.bounds.y - mesh_bounds.min.y * -mesh_scale.y},
            0.0f,
            mesh_scale);
        DrawMesh((Mesh*)e.resource);
        break;
    }
    }
}

extern float Evaluate(const StyleLength& length, float parent_value);

static Vec2 MeasureContent(Element& e, const Vec2& available_size)
{
    (void)available_size;

    switch (e.type)
    {
    case ELEMENT_TYPE_LABEL:
        if (e.resource != nullptr)
            return GetSize((TextMesh*)e.resource);
        break;

    case ELEMENT_TYPE_IMAGE:
        if (e.resource != nullptr)
        {
            Texture* texture = GetTexture((Material*)e.resource, 0);
            if (texture)
                return ToVec2(GetSize(texture));
        }
        break;

    case ELEMENT_TYPE_MESH:
        if (e.resource != nullptr)
            return GetSize((Mesh*)e.resource);
        break;

    default:
        break;
    }

    return { 0, 0 };
}

u32 MeasureElement(u32 element_index, const Vec2& available_size)
{
    Element& e = g_ui.elements[element_index++];
    Style& style = e.style;
    auto& measured_size = e.measured_size;

    Vec2 content_available = available_size;
    if (IsFixed(style.padding_left))
        content_available.x -= Evaluate(style.padding_left, 0);
    if (IsFixed(style.padding_right))
        content_available.x -= Evaluate(style.padding_right, 0);
    if (IsFixed(style.padding_top))
        content_available.y -= Evaluate(style.padding_top, 0);
    if (IsFixed(style.padding_bottom))
        content_available.y -= Evaluate(style.padding_bottom, 0);

    Vec2 content_measured_size = MeasureContent(e, content_available);

    // Children get the content size after padding as their available size
    Vec2 child_available_size = content_available;

    Vec2 child_measured_size = { 0.0f, 0.0f };
    for (u32 child_index=0, child_count = e.child_count; child_index<child_count; child_index++)
    {
        Element& child = g_ui.elements[element_index];
        Style& child_style = child.style;

        if (child_style.position.value == POSITION_TYPE_ABSOLUTE)
        {
            // Absolute children get the full available size of the parent
            element_index = MeasureElement(element_index, available_size);
            continue;
        }

        element_index = MeasureElement(element_index, available_size);

        // Calculate child size including margins for parent's auto-sizing
        auto child_width_with_margins = child.measured_size.x;
        auto child_height_with_margins = child.measured_size.y;

        // Add margins to child size for parent calculations
        if (IsFixed(child_style.margin_left))
            child_width_with_margins += Evaluate(child_style.margin_left, child_available_size.x);
        if (IsFixed(child_style.margin_right))
            child_width_with_margins += Evaluate(child_style.margin_right, child_available_size.x);
        if (IsFixed(child_style.margin_top))
            child_height_with_margins += Evaluate(child_style.margin_top, child_available_size.y);
        if (IsFixed(child_style.margin_bottom))
            child_height_with_margins += Evaluate(child_style.margin_bottom, child_available_size.y);

        if (IsAuto(style.width))
        {
            if (style.flex_direction.value == FLEX_DIRECTION_COL)
                child_measured_size.x += child_width_with_margins;
            else
                child_measured_size.x = Max(child_width_with_margins, child_measured_size.x);
        }

        if (IsAuto(style.height))
        {
            if (style.flex_direction.value == FLEX_DIRECTION_ROW)
                child_measured_size.y += child_height_with_margins;
            else
                child_measured_size.y = Max(child_height_with_margins, child_measured_size.y);
        }
    }

    measured_size = Vec2(0.0f, 0.0f);

    if (IsAuto(style.width))
    {
        measured_size.x = Max(content_measured_size.x, child_measured_size.x);
        // Add padding to the measured size
        if (IsFixed(style.padding_left))
            measured_size.x += Evaluate(style.padding_left, available_size.x);
        if (IsFixed(style.padding_right))
            measured_size.x += Evaluate(style.padding_right, available_size.x);
    }
    else
        measured_size.x = Evaluate(style.width, available_size.x);

    if (IsAuto(style.height))
    {
        measured_size.y = Max(content_measured_size.y, child_measured_size.y);
        // Add padding to the measured size
        if (IsFixed(style.padding_top))
            measured_size.y += Evaluate(style.padding_top, available_size.y);
        if (IsFixed(style.padding_bottom))
            measured_size.y += Evaluate(style.padding_bottom, available_size.y);
    }
    else
        measured_size.y = Evaluate(style.height, available_size.y);

    return element_index;
}

void BeginUI(u32 ref_width, u32 ref_height)
{
    g_ui.ref_size = { (i32)ref_width, (i32)ref_height };
    g_ui.element_stack_count = 0;
    g_ui.element_count = 0;
    g_ui.in_frame = true;

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
    if (WasButtonPressed(g_ui.input, MOUSE_LEFT))
    {
        for (int i=g_ui.element_count; i>0; i--)
        {
            Element& e = g_ui.elements[i-1];
            if (e.type != ELEMENT_TYPE_CANVAS && Contains(e.bounds, mouse))
            {
                ConsumeButton(MOUSE_LEFT);

                if (e.input_func != nullptr)
                {
                    ElementInput input;
                    input.button = MOUSE_LEFT;
                    input.mouse_position = mouse;
                    input.user_data = e.input_user_data;
                    input.bounds = e.bounds;
                    if (e.input_func(input))
                    {
                        return;
                    }
                }
            }
        }
    }
}

void EndUI()
{
    //Vec2Int screen_size = GetScreenSize();
    Vec2 screen_size_f = g_ui.ortho_size; //  Vec2((f32)screen_size.x, (f32)screen_size.y);
    Rect screen_bounds = { 0, 0, screen_size_f.x, screen_size_f.y };
    for (u32 element_index=0; element_index < g_ui.element_count; )
        element_index = MeasureElement(element_index, screen_size_f);
    for (u32 element_index=0; element_index < g_ui.element_count; )
        element_index = Layout(element_index, screen_bounds);

    g_ui.in_frame = false;

    HandleInput();
}

void DrawUI()
{
    assert(!g_ui.in_frame);

    for (u32 i = 0; i < g_ui.element_count; i++)
        RenderElement(g_ui.elements[i]);
}

static u32 LayoutChildren(
    u32 element_index,
    float content_left,
    float content_top,
    float content_width,
    float content_height)
{
    Element& e = g_ui.elements[element_index++];
    if (e.child_count == 0)
        return element_index;

    FlexDirection flex_direction = e.style.flex_direction.value;
    bool is_column = flex_direction == FLEX_DIRECTION_COL;

    // Pass 1: Calculate total intrinsic size and count auto margins
    f32 totalIntrinsicSize = 0.0f;
    i32 auto_margin_count = 0;

    for (u32 child_index=0, child_count=e.child_count; child_index<child_count; child_index++)
    {
	Element& child = g_ui.elements[element_index];
	Style& child_style = child.style;

        if (child_style.position.value == POSITION_TYPE_ABSOLUTE)
            continue;

        // Add child's measured size (which does NOT include margins)
        totalIntrinsicSize += is_column
            ? child.measured_size.x
            : child.measured_size.y;

        // Add fixed margins to total size and count auto margins
        if (is_column)
        {
            if (IsAuto(child_style.margin_left))
                auto_margin_count++;
            else
                totalIntrinsicSize += Evaluate(child_style.margin_left, content_width);

            if (IsAuto(child_style.margin_right))
                auto_margin_count++;
            else
                totalIntrinsicSize += Evaluate(child_style.margin_right, content_width);
        }
        else
        {
            if (IsAuto(child_style.margin_top))
                auto_margin_count++;
            else
                totalIntrinsicSize += Evaluate(child_style.margin_top, content_height);

            if (IsAuto(child_style.margin_bottom))
                auto_margin_count++;
            else
                totalIntrinsicSize += Evaluate(child_style.margin_bottom, content_height);
        }
    }

    // Pass 2: Calculate auto margin size
    float mainAxisSize = is_column ? content_width : content_height;
    float remainingSpace = mainAxisSize - totalIntrinsicSize;
    float flex_margin_size = auto_margin_count > 0 && remainingSpace > 0 ? remainingSpace / auto_margin_count : 0.0f;

    // Pass 3: Layout children with resolved margins
    auto current_offset = 0.0f;
    for (u32 child_index=0, child_count=e.child_count; child_index<child_count; child_index++)
    {
        Element& child = g_ui.elements[element_index];
        Style& child_style = child.style;

        if (child_style.position.value == POSITION_TYPE_ABSOLUTE)
        {
            element_index = Layout(element_index, {content_left,content_top,content_width,content_height});
            continue;
        }
        // Calculate resolved margins
        float margin_start;
        float margin_end;
        if (is_column)
        {
            margin_start = IsAuto(child_style.margin_left)
                ? flex_margin_size
                : Evaluate(child_style.margin_left, content_width);
            margin_end = IsAuto(child_style.margin_right)
                ? flex_margin_size
                : Evaluate(child_style.margin_right, content_width);
        }
        else
        {
            margin_start = IsAuto(child_style.margin_top)
                ? flex_margin_size
                : Evaluate(child_style.margin_top, content_height);
            margin_end = IsAuto(child_style.margin_bottom)
                ? flex_margin_size
                : Evaluate(child_style.margin_bottom, content_height);
        }

        // Don't add margin to offset - give child the full space including margins
        // The child will position itself within this space using its own margins

        // Create child bounds that include space for margins
        Rect child_bounds;
        if (is_column)
        {
            float child_total_width = margin_start + child.measured_size.x + margin_end;
            child_bounds = {
                content_left + current_offset,
                content_top,
                child_total_width,
                content_height
            };
            current_offset += child_total_width;
        }
        else
        {
            float child_total_height = margin_start + child.measured_size.y + margin_end;
            child_bounds = {
                content_left,
                content_top + current_offset,
                content_width,
                child_total_height
            };
            current_offset += child_total_height;
        }

        element_index = Layout(element_index, child_bounds);
    }

    return element_index;
}


static void LayoutAxis(
    const StyleLength& margin_min,
    const StyleLength& margin_max,
    const StyleLength& size,
    float measured_size,
    float available_size,
    float& resolved_margin_min,
    float& resolved_margin_max,
    float& resolved_size)
{
    resolved_margin_min = 0;
    resolved_margin_max = 0;
    resolved_size = 0;

    // Calculate the total contribution of auto margins and size (treated as flex(1))
    float contrib = 0.0f;
    if (IsAuto(margin_min))
        contrib += 1.0f;
    else
        resolved_margin_min = Evaluate(margin_min, available_size);

    if (IsAuto(margin_max))
        contrib += 1.0f;
    else
        resolved_margin_max = Evaluate(margin_max, available_size);

    // Handle size calculation accounting for margins
    if (IsAuto(size))
    {
        resolved_size = measured_size;
    }
    else
    {
        // For percentage sizes, reduce available space by margins first
        bool has_percent_size = size.unit == STYLE_LENGTH_UNIT_PERCENT;
        float total_margin_space = resolved_margin_min + resolved_margin_max;

        if (has_percent_size && total_margin_space > 0.0f)
        {
            float available_for_size = available_size - total_margin_space;
            if (available_for_size > 0.0f)
                resolved_size = Evaluate(size, available_for_size);
            else
                resolved_size = 0.0f;
        }
        else
        {
            resolved_size = Evaluate(size, available_size);
        }
    }

    float remaining_space = available_size - resolved_margin_min - resolved_margin_max - resolved_size;

    if (contrib > 0.0f && remaining_space > 0.0f)
    {
        contrib = 1.0f / contrib;
        if (IsAuto(margin_min))
            resolved_margin_min = 1.0f * contrib * remaining_space;
        if (IsAuto(margin_max))
            resolved_margin_max = 1.0f * contrib * remaining_space;
    }
}

u32 Layout(u32 element_index, Rect parent_bounds)
{
    Element& e = g_ui.elements[element_index];
    Style& style = e.style;
    Rect& bounds = e.bounds;
    Vec2& measured_size = e.measured_size;

    if (e.type == ELEMENT_TYPE_CANVAS)
        parent_bounds = { 0, 0, e.style.width.value, e.style.height.value };

    float hmin = 0.0f;
    float hmax = 0.0f;
    float hsize = 0.0f;
    LayoutAxis(
        style.margin_left,
        style.margin_right,
        style.width,
        measured_size.x,
        parent_bounds.width,
        hmin,
        hmax,
        hsize);

    float vmin = 0.0f;
    float vmax = 0.0f;
    float vsize = 0.0f;
    LayoutAxis(
        style.margin_top,
        style.margin_bottom,
        style.height,
        measured_size.y,
        parent_bounds.height,
        vmin,
        vmax,
        vsize);

    bounds = {
        parent_bounds.x + hmin,
        parent_bounds.y + vmin,
        hsize,
        vsize
    };

    // Calculate content area after padding (relative to Element* bounds)
    float content_left = 0;
    float content_top = 0;
    float content_width = hsize;
    float content_height = vsize;

    if (IsFixed(style.padding_left))
    {
        float paddingValue = Evaluate(style.padding_left, 0);
        content_left += paddingValue;
        content_width -= paddingValue;
    }
    if (IsFixed(style.padding_right))
        content_width -= Evaluate(style.padding_right, 0);

    if (IsFixed(style.padding_top))
    {
        float paddingValue = Evaluate(style.padding_top, 0);
        content_top += paddingValue;
        content_height -= paddingValue;
    }

    if (IsFixed(style.padding_bottom))
        content_height -= Evaluate(style.padding_bottom, 0);

    element_index = LayoutChildren(
        element_index,
        bounds.x + content_left,
        bounds.y + content_top,
        content_width,
        content_height);

    return element_index;
}

static u64 GetMeshHash(const TextRequest& request)
{
    return Hash(Hash(request.text), (u64)request.font, (u64)request.font_size);
}

void Label(const char* text, const StyleId& style_id)
{
    BeginElement(ELEMENT_TYPE_LABEL, style_id);

    Element& e = GetCurrentElement();

    TextRequest r = {};
    r.font = e.style.font.id >= 0 ? FONT[e.style.font.id] : nullptr;
    r.font_size = e.style.font_size.value;
    SetValue(r.text, text);
    u64 mesh_key = GetMeshHash(r);

    CachedTextMesh* c = (CachedTextMesh*)GetValue(g_ui.text_mesh_cache, mesh_key);
    if (c == nullptr)
    {
        if (TextMesh* tm = CreateTextMesh(ALLOCATOR_DEFAULT, r))
        {
            e.resource = tm;
            e.material = GetMaterial(tm);
            c = (CachedTextMesh*)SetValue(g_ui.text_mesh_cache, mesh_key);
            c->last_frame = 0;
            c->text_mesh = tm;
        }
    }
    else
    {
        e.resource = c->text_mesh;
        e.material = GetMaterial(c->text_mesh);
    }

    EndElement();
}

void Image(Material* material, const StyleId& style_id)
{
    BeginElement(ELEMENT_TYPE_IMAGE, style_id);
    Element& e = GetCurrentElement();
    e.resource = material;
    EndElement();
}

void MeshElement(Mesh* mesh, Material* material, const StyleId& style_id)
{
    assert(mesh);
    assert(material);

    BeginElement(ELEMENT_TYPE_MESH, style_id);
    Element& e = GetCurrentElement();
    e.resource = mesh;
    e.material = material;
    EndElement();
}

bool IsMouseOverElement()
{
    return (GetCurrentElement().flags & ELEMENT_FLAG_HOVER) != 0;
}

bool DidMouseEnterElement()
{
    return (GetCurrentElement().flags & ELEMENT_FLAG_MOUSE_ENTER) != 0;
}

bool DidMouseLeaveElement()
{
    return (GetCurrentElement().flags & ELEMENT_FLAG_MOUSE_LEAVE) != 0;
}

void SetElementStyle(const StyleId& style_id)
{
    GetCurrentElement().style = GetStyle(style_id);
}

void InitUI()
{
    g_ui.camera = CreateCamera(ALLOCATOR_DEFAULT);
    g_ui.element_quad = CreateElementQuad(ALLOCATOR_DEFAULT);
    g_ui.element_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI);
    g_ui.vignette_material = CreateMaterial(ALLOCATOR_DEFAULT, SHADER_UI_VIGNETTE);
    g_ui.input = CreateInputSet(ALLOCATOR_DEFAULT);
    EnableButton(g_ui.input, MOUSE_LEFT);
    SetTexture(g_ui.element_material, TEXTURE_WHITE);

    Init(g_ui.text_mesh_cache, g_ui.text_mesh_cache_keys, g_ui.text_mesh_cache_values, 1024, sizeof(CachedTextMesh));
}

void ShutdownUI()
{
    g_ui = {};
}


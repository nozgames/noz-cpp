//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

void MarkDirty(Canvas* canvas);
void RenderElementQuad(const color_t& color, Texture* texture);


struct ElementFlags
{
    u32 value;
} __attribute__((packed));

constexpr u32 ELEMENT_FLAG_VISIBLE = 1 << 0;

struct ElementImpl : Object
{
    Canvas* canvas;
    const name_t* name;
    ElementFlags flags;
    Style style;
    Rect bounds;
    vec2 measured_size;
    Element* parent = nullptr;
    LinkedList children;
    LinkedListNode node_child;
    PseudoState PseudoState;
    mat4 local_to_world;
    bool has_explicit_pseudo_state = false;
};

static_assert(ELEMENT_BASE_SIZE == sizeof(ElementImpl));

const ElementTraits* g_element_traits[TYPE_COUNT] = {};

static ElementImpl* Impl(Element* element) { return (ElementImpl*)CastToBase(element, TYPE_ELEMENT); }

static bool Has(PseudoState state, PseudoState mask) { return (state & mask) == mask;}
static bool HasAny(PseudoState state, PseudoState mask) { return (state & mask) != 0; }
static PseudoState GetEffectivePseudoState(ElementImpl* impl);
static void ApplyStyle(ElementImpl* impl);
static void Measure(ElementImpl* impl, const vec2& available_size);
static vec2 MeasureContent(ElementImpl* impl, const vec2& available_size);
static void RenderBackground(ElementImpl* impl, const mat4& canvas_transform);
static void Layout(ElementImpl* impl, const Rect& parent_bounds);
static void layout_children(ElementImpl* impl, float content_left, float content_top, float content_width, float content_height);
static void LayoutAxis(
    ElementImpl* impl,
    const StyleLength& margin_min,
    const StyleLength& margin_max,
    const StyleLength& size,
    float measured_size,
    float available_size,
    float& resolved_margin_min,
    float& resolved_margin_max,
    float& resolved_size);

Element* CreateRootElement(Allocator* allocator, Canvas* canvas, const name_t* id)
{
    assert(canvas);

    auto element = CreateElement(allocator, id);
    auto impl = Impl(element);
    impl->canvas = canvas;
    impl->flags.value = ELEMENT_FLAG_VISIBLE;
    impl->style = GetDefaultStyle();
    impl->style.width = { STYLE_KEYWORD_INLINE, STYLE_LENGTH_UNIT_PERCENT, 1.0f };
    impl->style.height = { STYLE_KEYWORD_INLINE, STYLE_LENGTH_UNIT_PERCENT, 1.0f };
    Init(impl->children, offsetof(ElementImpl, node_child));
    return element;
}

Element* CreateElement(Allocator* allocator, size_t element_size, type_t element_type, const name_t* name)
{
    auto element = (Element*)CreateObject(allocator, element_size, element_type, TYPE_ELEMENT);
    auto impl = Impl(element);
    impl->name = name;
    impl->style = GetDefaultStyle();
    impl->flags.value = ELEMENT_FLAG_VISIBLE;
    Init(impl->children, offsetof(ElementImpl, node_child));
    return element;
}

Element* CreateElement(Allocator* allocator, const name_t* name)
{
    return CreateElement(allocator, sizeof(ElementImpl), TYPE_ELEMENT, name);
}

void SetParent(Element* element, Canvas* canvas)
{
    SetParent(element, GetRootElement(canvas));
}

void SetParent(Element* element, Element* parent)
{
    auto impl = Impl(element);
    assert(parent);

    if (impl->parent == parent)
        return;

    if (impl->parent)
        RemoveFromParent(element);

    auto parent_impl = Impl(parent);
    PushBack(parent_impl->children, element);
    impl->parent = parent;
}

void AddChild(Element* element, Element* child)
{
    SetParent(child, element);
}

void RemoveFromParent(Element* element)
{
    auto impl = Impl(element);
    if (!impl->parent)
        return;

    auto parent_impl = Impl(impl->parent);
    Remove(parent_impl->children, element);
    impl->parent = nullptr;
}

bool IsInCanvas(Element* element) { return Impl(element)->canvas; }
Canvas* GetCanvas(Element* element) { return Impl(element)->canvas; }
bool IsVisible(Element* element) { return (Impl(element)->flags.value & ELEMENT_FLAG_VISIBLE) != 0; }

void SetVisible(Element* element, bool value)
{
    auto impl = Impl(element);

    if (IsVisible(element) == value)
        return;

    if (value)
        impl->flags.value |= ELEMENT_FLAG_VISIBLE;
    else
        impl->flags.value &= ~ELEMENT_FLAG_VISIBLE;
}

void MarkDirty(Element* element)
{
    MarkDirty(Impl(element)->canvas);
}

static vec2 MeasureContent(ElementImpl* impl, const vec2& available_size)
{
    auto traits = GetElementTraits((Element*)impl);
    if (traits->measure_content)
        return traits->measure_content((Element*)impl, available_size, impl->style);

    return VEC2_ZERO;
}

static bool ShouldInherit(PseudoState state)
{
    return HasAny(state, PSEUDO_STATE_SELECTED | PSEUDO_STATE_DISABLED | PSEUDO_STATE_ACTIVE);
}

static PseudoState GetHighestPriority(PseudoState mask)
{
    if (Has(mask, PSEUDO_STATE_DISABLED)) return PSEUDO_STATE_DISABLED;
    if (Has(mask, PSEUDO_STATE_SELECTED) && Has(mask, PSEUDO_STATE_HOVER))
        return PSEUDO_STATE_SELECTED | PSEUDO_STATE_HOVER;
    if (Has(mask, PSEUDO_STATE_SELECTED)) return PSEUDO_STATE_SELECTED;
    if (Has(mask, PSEUDO_STATE_PRESSED)) return PSEUDO_STATE_PRESSED;
    if (Has(mask, PSEUDO_STATE_FOCUSED)) return PSEUDO_STATE_FOCUSED;
    if (Has(mask, PSEUDO_STATE_ACTIVE)) return PSEUDO_STATE_ACTIVE;
    if (Has(mask, PSEUDO_STATE_HOVER)) return PSEUDO_STATE_HOVER;
    if (Has(mask, PSEUDO_STATE_CHECKED)) return PSEUDO_STATE_CHECKED;
    return PSEUDO_STATE_NONE;
}

float Evaluate(const StyleLength& length, float parent_value)
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

void Measure(ElementImpl* impl, const vec2& available_size)
{
    auto& style = impl->style;
    auto& measured_size = impl->measured_size;

    // Calculate available size for content after padding
    auto content_available = available_size;
    if (IsFixed(style.padding_left))
        content_available.x -= Evaluate(style.padding_left, 0);
    if (IsFixed(style.padding_right))
        content_available.x -= Evaluate(style.padding_right, 0);
    if (IsFixed(style.padding_top))
        content_available.y -= Evaluate(style.padding_top, 0);
    if (IsFixed(style.padding_bottom))
        content_available.y -= Evaluate(style.padding_bottom, 0);

    auto content_measured_size = MeasureContent(impl, content_available);

    // Children get the content size after padding as their available size
    auto child_available_size = content_available;

    // Now measure the children
    auto child_measured_size = vec2(0.0f, 0.0f);
    for (auto child = (Element*)GetFront(impl->children); child; child = (Element*)GetNext(impl->children, child))
    {
	auto child_impl = Impl(child);
	auto& child_style = child_impl->style;

        Measure(child_impl, child_available_size);

        // Calculate child size including margins for parent's auto-sizing
        auto child_width_with_margins = child_impl->measured_size.x;
        auto child_height_with_margins = child_impl->measured_size.y;

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
            if (style.flex_direction.value == FLEX_DIRECTION_COL ||
                style.flex_direction.value == FLEX_DIRECTION_COL_REVERSE)
                child_measured_size.x += child_width_with_margins;
            else
                child_measured_size.x = max(child_width_with_margins, child_measured_size.x);
        }

        if (IsAuto(style.height))
        {
            if (style.flex_direction.value == FLEX_DIRECTION_ROW ||
                style.flex_direction.value == FLEX_DIRECTION_ROW_REVERSE)
                child_measured_size.y += child_height_with_margins;
            else
                child_measured_size.y = max(child_height_with_margins, child_measured_size.y);
        }
    }

    measured_size = vec2(0.0f, 0.0f);

    if (IsAuto(style.width))
    {
        measured_size.x = max(content_measured_size.x, child_measured_size.x);
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
        measured_size.y = max(content_measured_size.y, child_measured_size.y);
        // Add padding to the measured size
        if (IsFixed(style.padding_top))
            measured_size.y += Evaluate(style.padding_top, available_size.y);
        if (IsFixed(style.padding_bottom))
            measured_size.y += Evaluate(style.padding_bottom, available_size.y);
    }
    else
        measured_size.y = Evaluate(style.height, available_size.y);

    // Do NOT add margins to measured_size - they are handled separately in layout
}

// @layout

void Layout(ElementImpl* impl, const Rect& parent_bounds)
{
	    auto& style = impl->style;
	    auto& bounds = impl->bounds;
	    auto& measured_size = impl->measured_size;

    float hmin = 0.0f;
    float hmax = 0.0f;
    float hsize = 0.0f;
    LayoutAxis(
        impl,
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
        impl,
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

    // Two-pass flex layout for proper auto margin handling
    // Pass in the world-space content bounds
    layout_children(
        impl,
        bounds.x + content_left,
        bounds.y + content_top,
        content_width,
        content_height);

    impl->local_to_world =
        translate(vec3(bounds.x, bounds.y, 0.0f)) *
        scale(vec3(bounds.width, bounds.height, 1.0f));
}

static void layout_children(
    ElementImpl* impl,
    float content_left,
    float content_top,
    float content_width,
    float content_height)
{
    assert(impl);
    auto child_count = GetCount(impl->children);
    if (child_count == 0)
        return;

    auto flex_direction = impl->style.flex_direction.value;
    auto is_column = (flex_direction == FLEX_DIRECTION_COL || flex_direction == FLEX_DIRECTION_COL_REVERSE);
    auto is_reverse = (flex_direction == FLEX_DIRECTION_ROW_REVERSE || flex_direction == FLEX_DIRECTION_COL_REVERSE);

    // Pass 1: Calculate total intrinsic size and count auto margins
    float totalIntrinsicSize = 0.0f;
    int auto_margin_count = 0;

    for (auto child = (Element*)GetFront(impl->children); child; child = (Element*)GetNext(impl->children, child))
    {
	auto child_impl = Impl(child);
	auto& child_style = child_impl->style;

        // Add child's measured size (which does NOT include margins)
        totalIntrinsicSize += is_column
            ? child_impl->measured_size.x
            : child_impl->measured_size.y;

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
    float flex_margin_size = (auto_margin_count > 0 && remainingSpace > 0) ? remainingSpace / auto_margin_count : 0.0f;

    // Pass 3: Layout children with resolved margins
    auto current_offset = 0.0f;

    // Set up iteration based on direction
    auto child_index = is_reverse ? (child_count - 1) : 0;
    auto increment = is_reverse ? -1 : 1;

    for (auto child = (Element*)GetFront(impl->children); child; child = (Element*)GetNext(impl->children, child))
    {
        auto child_impl = Impl(child);;
        auto& child_style = child_impl->style;

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
        Rect child_bounds = {};
        if (is_column)
        {
            float child_total_width = margin_start + child_impl->measured_size.x + margin_end;
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
            float child_total_height = margin_start + child_impl->measured_size.y + margin_end;
            child_bounds = {
                content_left,
                content_top + current_offset,
                content_width,
                child_total_height
            };
            current_offset += child_total_height;
        }

        Layout(child_impl, child_bounds);

        // Move to next child
        child_index += increment;
    }
}

static void LayoutAxis(
    ElementImpl* impl,
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
        bool has_percent_size = (size.unit == STYLE_LENGTH_UNIT_PERCENT);
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

static void RenderBackground(ElementImpl* impl, const mat4& canvas_transform)
{
    BindTransform(canvas_transform * impl->local_to_world);
    RenderElementQuad(impl->style.background_color.value, nullptr);
}

// @style

static void ApplyStyle(ElementImpl* impl, const name_t* id)
{
    if (!id)
        return;

    auto style_sheet = GetStyleSheet(impl->canvas);
    if (!style_sheet)
        return;

    // Try to get pseudo state style first (e.g., "button:hover")
    PseudoState effective_state = GetEffectivePseudoState(impl);
    while (true)
    {
        auto priority_state = GetHighestPriority(effective_state);
        effective_state &= ~priority_state;

        Style style;
        if (!GetStyle(style_sheet, impl->name, priority_state, &style))
        {
            if (priority_state == PSEUDO_STATE_NONE)
                return;

            continue;
        }

        MergeStyles(impl->style, style, true);
        break;
    }

    // Optional trait callback
    auto traits = GetElementTraits((Element*)impl);
    if (traits->on_apply_style)
	traits->on_apply_style((Element*)impl, impl->style);
}

static void ApplyStyle(ElementImpl* impl)
{
    if (!impl->canvas)
        return;

    ApplyStyle(impl, impl->name);

    for (auto child = (Element*)GetFront(impl->children); child; child = (Element*)GetNext(impl->children, child))
    {
        auto child_impl = Impl(child);
        if (!child_impl->canvas)
	child_impl->canvas = impl->canvas;
        ApplyStyle(child_impl);
    }
}

void SetPseudoState(Element* element, PseudoState state, bool value)
{
    auto impl = Impl(element);
    auto old_state = impl->PseudoState;
    auto new_state = impl->PseudoState;
    if (value)
        new_state |= state;
    else
        new_state &= ~state;

    if (old_state == new_state)
        return;

    impl->PseudoState = new_state;
    impl->has_explicit_pseudo_state = new_state != PSEUDO_STATE_NONE;

    MarkDirty((Element*)impl);
}

static PseudoState GetEffectivePseudoState(ElementImpl* impl)
{
    if (!impl)
	return PSEUDO_STATE_NONE;

    // If this Element* has an explicit pseudo state, use it
    if (impl->has_explicit_pseudo_state)
        return impl->PseudoState;

    // Check if we should inherit from parent based on parent's pseudo state
    auto effective_state = GetEffectivePseudoState((ElementImpl*)impl->parent);

    // Only inherit states that are designed to cascade down
    if (ShouldInherit(effective_state))
        return effective_state;

    return PSEUDO_STATE_NONE;
}

static bool HasBackground(ElementImpl* impl)
{
    return impl->style.background_color.value.a > 0;
}

void RenderElement(Element* element, const mat4& canvas_transform)
{
    if (!IsVisible(element))
        return;

    auto impl = Impl(element);
    auto traits = GetElementTraits(GetType(element));
    auto should_render_background = HasBackground(impl);
    auto should_render_content = traits->render_content;

    if (should_render_background)
        RenderBackground(impl, canvas_transform);

    if (should_render_content)
    {
        BindTransform(canvas_transform * translate(vec3(impl->bounds.x, impl->bounds.y, 0.0f)));
        traits->render_content(element, impl->style);
    }

    for (auto child = (Element*)GetFront(impl->children); child; child = (Element*)GetNext(impl->children, child))
        RenderElement(child, canvas_transform);
}

// @render
void RenderElements(Element* element, const mat4& canvas_transform, const vec2& canvas_size, bool is_dirty)
{
    auto impl = Impl(element);
    if (is_dirty)
    {
        ApplyStyle(impl);
        Measure(impl, canvas_size);
        Layout(impl, {0.0f, 0.0f, canvas_size.x, canvas_size.y});
    }
    RenderElement(element, canvas_transform);
}

const Style& GetStyle(Element* element)
{
    return Impl(element)->style;
}

void ElementDestructor(Object* o)
{
    auto element = Impl((Element*)o);
    auto traits = GetElementTraits(GetType(o));

    //RemoveFromParent(entity, false);
}

void SetElementTraits(type_t id, const ElementTraits* traits)
{
    g_element_traits[id] = traits;
}

Element* GetFirstChild(Element* element)
{
    return (Element*)GetFront(Impl(element)->children);
}

Element* GetNextChild(Element* element, Element* child)
{
    return (Element*)GetNext(Impl(element)->children, child);
}

const name_t* GetName(Element* element)
{
    return Impl(element)->name;
}

void InitElement()
{
    static TypeTraits element_type_traits = {
        .destructor = ElementDestructor,
    };
    SetTypeTraits(TYPE_ELEMENT, &element_type_traits);

    // To prevent the need for nullptr checks just initialize all to default
    static ElementTraits default_traits = {};
    for (int i=0; i<TYPE_COUNT; i++)
        g_element_traits[i] = &default_traits;
}

void ShutdownElement()
{
}


#ifdef NOZ_EDITOR
void WriteInspectorElement(Stream* stream, Element* element)
{
    auto impl = Impl(element);
    BeginInspectorObject(stream, GetType(element), GetValue(GetName(element)));
    WriteInspectorProperty(stream, "enabled", IsEnabled(impl->canvas));

    if (auto traits = GetElementTraits(element); traits->editor_inspect)
        traits->editor_inspect(element, stream);

    for (auto child=GetFirstChild(element); child; child=GetNextChild(element, child))
        WriteInspectorElement(stream, child);

    EndInspectorObject(stream);
}
#endif
//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct LabelImpl
{
    ELEMENT_BASE;
    text_t text;
    TextMesh* mesh;
    u64 mesh_hash;
};

static LabelImpl* Impl(Label* label) { return (LabelImpl*)Cast(label, TYPE_LABEL); }
static u64 GetMeshHash(const TextRequest& request)
{
    return Hash(Hash(request.text), (u64)request.font, (u64)request.font_size);
}

static vec2 LabelMeasureContent(Element* element, const vec2& available_size, const Style& style);
static void LabelRenderContent(Element* element, const Style& style);
static void CreateTextMesh(LabelImpl* impl, const Style& style);
static void LabelOnApplyStyle(Element* element, const Style& style);

static vec2 LabelMeasureContent(Element* element, const vec2& available_size, const Style& style)
{
    auto impl = Impl((Label*)element);
    if (impl->mesh)
        return GetSize(impl->mesh);

    if (IsEmpty(impl->text))
        return VEC2_ZERO;

    return MeasureText(impl->text, GetDefaultFont(), (float)style.font_size.value);
}

static void LabelRenderContent(Element* element, const Style& style)
{
    auto impl = Impl((Label*)element);

    if (IsEmpty(impl->text))
        return;

    if (!impl->mesh)
        CreateTextMesh(impl, style);

    if (!impl->mesh)
        return;

    auto mesh = GetMesh(impl->mesh);
    if (!mesh)
        return;

    BindColor(style.color.value);
    BindMaterial(GetMaterial(impl->mesh));
    DrawMesh(mesh);
}

static void MarkDirty(LabelImpl* impl)
{
    Destroy(impl->mesh);
    impl->mesh = nullptr;
    MarkDirty((Element*)impl);
}

void SetText(Label* label, const char* text)
{
    auto impl = Impl(label);
    SetValue(impl->text, text);
    MarkDirty(impl);
}

void SetText(Label* label, const text_t& text)
{
    auto impl = Impl(label);
    SetValue(impl->text, text);
    MarkDirty(impl);
}

const text_t& GetText(Label* label)
{
    return Impl(label)->text;
}

static void CreateTextMesh(LabelImpl* impl, const Style& style)
{
    assert(impl);
    assert(!impl->mesh);

    TextRequest request = {};
    SetValue(request.text, impl->text);
    request.font = GetDefaultFont();
    request.font_size = style.font_size.value;

    // todo: better allocator?
    impl->mesh = CreateTextMesh(ALLOCATOR_DEFAULT, request);
    impl->mesh_hash = GetMeshHash(request);
}

static void LabelOnApplyStyle(Element* element, const Style& style)
{
    auto impl = Impl((Label*)element);
    TextRequest request = {};
    SetValue(request.text, impl->text);
    request.font = GetDefaultFont();
    request.font_size = style.font_size.value;

    auto mesh_hash = GetMeshHash(request);
    if (mesh_hash == impl->mesh_hash)
        return;

    MarkDirty(impl);
}

Label* CreateLabel(Allocator* allocator, const char* text, const name_t* id)
{
    auto label = (Label*)CreateElement(allocator, sizeof(LabelImpl), TYPE_LABEL, id);
    auto impl = Impl(label);
    SetValue(impl->text, text);
    return label;
}

Label* CreateLabel(Allocator* allocator, const text_t& text, const name_t* id)
{
    return CreateLabel(allocator, text.value, id);
}

Label* CreateLabel(Allocator* allocator, const name_t* id)
{
    return CreateLabel(allocator, "", id);
}

void InitLabel()
{
    static const ElementTraits traits =
    {
        .measure_content = LabelMeasureContent,
        .render_content = LabelRenderContent,
        .on_apply_style = LabelOnApplyStyle
    };

    SetElementTraits(TYPE_LABEL, &traits);
}


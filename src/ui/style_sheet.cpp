//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct StyleSheetImpl : StyleSheet
{
    Map styles;
#ifdef _HOTLOAD
    u64* hotload_keys;
    Style* hotload_styles;
#endif
};

static u64 GetStyleKey(const Name* id, PseudoState pseudo_state) { return Hash(Hash(id), pseudo_state); }

static void LoadStyles(StyleSheetImpl* impl, Stream* stream, u32 style_count, u64* keys, Style* styles)
{
    for (u32 i=0; i < style_count; i++)
        DeserializeStyle(stream, styles[i]);

    char id[256];
    for (u32 i=0; i < style_count; i++)
    {
        ReadString(stream, id, 256);
        auto pseudo_state = (PseudoState)ReadU32(stream);
        keys[i] = GetStyleKey(GetName(id), pseudo_state);
    }

    Init(impl->styles, keys, styles, style_count, sizeof(Style), style_count);
}

Asset* LoadStyleSheet(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name)
{
    auto style_count = ReadU32(stream);
    auto keys_size = style_count * sizeof(u64);
    auto style_size = style_count * sizeof(Style);

    auto* sheet = (StyleSheet*)Alloc(allocator, sizeof(StyleSheetImpl) + style_size + keys_size);
    if (!sheet)
        return nullptr;

    StyleSheetImpl* impl = static_cast<StyleSheetImpl*>(sheet);
    impl->name = name;

    auto keys = (u64*)(impl + 1);
    auto styles = (Style*)(keys + style_count);
    LoadStyles(impl, stream, style_count, keys, styles);

    return sheet;
}

bool GetStyle(StyleSheet* sheet, const Name* id, PseudoState pseudo_state, Style* result)
{
    u64 style_key = GetStyleKey(id, pseudo_state);
    StyleSheetImpl* impl = static_cast<StyleSheetImpl*>(sheet);
    Style* style = (Style*)GetValue(impl->styles, style_key);
    if (!style)
        return false;

    *result = *style;
    return true;
}

const Style& GetStyle(StyleSheet* sheet, const Name* id, PseudoState pseudo_state)
{
    u64 style_key = GetStyleKey(id, pseudo_state);
    StyleSheetImpl* impl = static_cast<StyleSheetImpl*>(sheet);
    Style* style = (Style*)GetValue(impl->styles, style_key);
    if (!style)
        return GetDefaultStyle();

    return *style;
}

bool HasStyle(StyleSheet* sheet, const Name* name, PseudoState pseudo_state)
{
    return HasKey(static_cast<StyleSheetImpl*>(sheet)->styles, GetStyleKey(name, pseudo_state));
}

#ifdef _HOTLOAD
void MarkAllCanvasesDirty(Entity* entity)
{
    if (GetType(entity) == TYPE_CANVAS)
        MarkDirty((Canvas*)entity);

    for (auto child = GetFirstChild(entity); child; child = GetNextChild(entity, child))
        MarkAllCanvasesDirty(child);
}

void ReloadStyleSheet(Object* asset, Stream* stream, const AssetHeader* header, const Name* name)
{
    auto impl = Impl((StyleSheet*)asset);

    if (impl->hotload_keys)
    {
        Free(ALLOCATOR_DEFAULT, impl->hotload_keys);
        Free(ALLOCATOR_DEFAULT, impl->hotload_styles);
    }

    auto style_count = ReadU32(stream);
    impl->hotload_keys = (u64*)Alloc(ALLOCATOR_DEFAULT, style_count * sizeof(u64));
    impl->hotload_styles = (Style*)Alloc(ALLOCATOR_DEFAULT, style_count * sizeof(Style));
    LoadStyles(impl, stream, style_count, impl->hotload_keys, impl->hotload_styles);

    // todo: we need to find all the canvases, maybe we track all canvases in canvas.cpp when _HOTLOAD
    //MarkAllCanvasesDirty(  GetSceneRoot());
}

#endif


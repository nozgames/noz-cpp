//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct StyleSheetImpl
{
    OBJECT_BASE;
    const name_t* name;
    Map styles;
#ifdef _HOTLOAD
    u64* hotload_keys;
    Style* hotload_styles;
#endif
};

static StyleSheetImpl* Impl(StyleSheet* s) { return (StyleSheetImpl*)Cast(s, TYPE_STYLE_SHEET); }

static u64 GetStyleKey(const name_t* id, PseudoState pseudo_state) { return Hash(Hash(id), pseudo_state); }

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

Object* LoadStyleSheet(Allocator* allocator, Stream* stream, AssetHeader* header, const name_t* name)
{
    auto style_count = ReadU32(stream);
    auto keys_size = style_count * sizeof(u64);
    auto style_size = style_count * sizeof(Style);

    auto* sheet = (StyleSheet*)CreateObject(allocator, sizeof(StyleSheetImpl) + style_size + keys_size, TYPE_STYLE_SHEET);
    if (!sheet)
        return nullptr;

    auto impl = Impl(sheet);
    impl->name = name;

    auto keys = (u64*)(impl + 1);
    auto styles = (Style*)(keys + style_count);
    LoadStyles(impl, stream, style_count, keys, styles);

    return sheet;
}

bool GetStyle(StyleSheet* sheet, const name_t* id, PseudoState pseudo_state, Style* result)
{
    auto style_key = GetStyleKey(id, pseudo_state);
    auto impl = Impl(sheet);
    auto style = (Style*)GetValue(impl->styles, style_key);
    if (!style)
        return false;

    *result = *style;
    return true;
}

const Style& GetStyle(StyleSheet* sheet, const name_t* id, PseudoState pseudo_state)
{
    auto style_key = GetStyleKey(id, pseudo_state);
    auto impl = Impl(sheet);
    auto style = (Style*)GetValue(impl->styles, style_key);
    if (!style)
        return GetDefaultStyle();

    return *style;
}

bool HasStyle(StyleSheet* sheet, const name_t* name, PseudoState pseudo_state)
{
    return HasKey(Impl(sheet)->styles, GetStyleKey(name, pseudo_state));
}

#ifdef _HOTLOAD
void MarkAllCanvasesDirty(Entity* entity)
{
    if (GetType(entity) == TYPE_CANVAS)
        MarkDirty((Canvas*)entity);

    for (auto child = GetFirstChild(entity); child; child = GetNextChild(entity, child))
        MarkAllCanvasesDirty(child);
}

void ReloadStyleSheet(Object* asset, Stream* stream, const AssetHeader* header, const name_t* name)
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

    MarkAllCanvasesDirty(GetSceneRoot());
}

#endif


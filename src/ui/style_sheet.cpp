//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct StyleSheetImpl : StyleSheet
{
    Map style_map;
    u64* keys;
    Style* styles;
};

static u64 GetStyleKey(const Name* id, PseudoState pseudo_state) { return Hash(Hash(id), pseudo_state); }

static void LoadStyles(StyleSheetImpl* impl, Allocator* allocator, Stream* stream, u32 style_count, const Name** name_table)
{
    impl->styles = (Style*)Alloc(allocator, style_count * sizeof(Style));
    impl->keys = (u64*)Alloc(allocator, style_count * sizeof(u64));

    for (u32 i=0; i < style_count; i++)
    {
        impl->styles[i] = GetDefaultStyle();
        DeserializeStyle(stream, impl->styles[i]);
    }

    for (u32 i=0; i < style_count; i++)
    {
        u32 name_index = ReadU32(stream);
        PseudoState pseudo_state = ReadU32(stream);
        impl->keys[i] = GetStyleKey(name_table[name_index], pseudo_state);
    }

    Init(impl->style_map, impl->keys, impl->styles, style_count, sizeof(Style), style_count);
}

Asset* LoadStyleSheet(Allocator* allocator, Stream* stream, AssetHeader* header, const Name* name, const Name** name_table)
{
    (void)header;

    auto style_count = ReadU32(stream);
    auto* sheet = (StyleSheet*)Alloc(allocator, sizeof(StyleSheetImpl));
    if (!sheet)
        return nullptr;

    StyleSheetImpl* impl = static_cast<StyleSheetImpl*>(sheet);
    impl->name = name;

    LoadStyles(impl, allocator, stream, style_count, name_table);

    return sheet;
}

bool GetStyle(StyleSheet* sheet, const Name* id, PseudoState pseudo_state, Style* result)
{
    u64 style_key = GetStyleKey(id, pseudo_state);
    StyleSheetImpl* impl = static_cast<StyleSheetImpl*>(sheet);
    Style* style = (Style*)GetValue(impl->style_map, style_key);
    if (!style)
        return false;

    *result = *style;
    return true;
}

const Style& GetStyle(StyleSheet* sheet, const Name* id, PseudoState pseudo_state)
{
    u64 style_key = GetStyleKey(id, pseudo_state);
    StyleSheetImpl* impl = static_cast<StyleSheetImpl*>(sheet);
    Style* style = (Style*)GetValue(impl->style_map, style_key);
    if (!style)
        return GetDefaultStyle();

    return *style;
}

bool HasStyle(StyleSheet* sheet, const Name* name, PseudoState pseudo_state)
{
    return HasKey(static_cast<StyleSheetImpl*>(sheet)->style_map, GetStyleKey(name, pseudo_state));
}

#ifdef NOZ_EDITOR

void ReloadStyleSheet(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table)
{
    (void)header;

    assert(asset);
    assert(stream);

    StyleSheetImpl* impl = static_cast<StyleSheetImpl*>(asset);

    Free(impl->styles);
    Free(impl->keys);

    LoadStyles(impl, ALLOCATOR_DEFAULT, stream, ReadU32(stream), name_table);
}

#endif


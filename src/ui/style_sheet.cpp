//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

struct StyleSheetImpl : StyleSheet
{
    u16 style_count;
    Style* styles;
};

static void LoadStyles(StyleSheetImpl* impl, Allocator* allocator, Stream* stream, u32 style_count, const Name** name_table)
{
    (void) name_table;

    impl->styles = (Style*)Alloc(allocator, style_count * sizeof(Style));
    impl->style_count = (u16)style_count;

    for (u32 i=0; i < style_count; i++)
    {
        Style& style = impl->styles[i];
        style = GetDefaultStyle();
        DeserializeStyle(stream, style);

        if (style.font.name[0] != 0)
        {
            char font_name[MAX_NAME_LENGTH] = {0};
            Copy(font_name, MAX_NAME_LENGTH, "font/");
            Copy(font_name + 5, MAX_NAME_LENGTH - 5, style.font.name);
            style.font.id = GetFontIndex(GetName(font_name));
        }
        else
            style.font.id = -1;
    }
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

const Style& GetStyle(const StyleId& style_id)
{
    if (style_id.style_sheet_id == 0xFFFF || style_id.id == 0xFFFF)
        return GetDefaultStyle();

    StyleSheetImpl* impl = (StyleSheetImpl*)STYLESHEET[style_id.style_sheet_id];
    assert(impl);
    assert(style_id.id < impl->style_count);
    return impl->styles[style_id.id];
}

#ifdef NOZ_EDITOR

void ReloadStyleSheet(Asset* asset, Stream* stream, const AssetHeader& header, const Name** name_table)
{
    (void)header;

    assert(asset);
    assert(stream);

    StyleSheetImpl* impl = static_cast<StyleSheetImpl*>(asset);
    Free(impl->styles);
    LoadStyles(impl, ALLOCATOR_DEFAULT, stream, ReadU32(stream), name_table);
}

#endif


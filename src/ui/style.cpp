//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include <noz/ui.h>

static Style g_default_style = {
    .flex_direction = { STYLE_KEYWORD_INHERIT, FLEX_DIRECTION_ROW },
    .width = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_AUTO, 0.0f },
    .height = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_AUTO, 0.0f },
    .background_color = { STYLE_KEYWORD_INHERIT, {0,0,0,0} },
    .color = { STYLE_KEYWORD_INHERIT, {1,1,1,1} },
    .font = { STYLE_KEYWORD_INHERIT, -1 },
    .font_size = { STYLE_KEYWORD_INHERIT, 16 },
    .margin_top = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .margin_left = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .margin_bottom = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .margin_right = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .padding_top = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .padding_left = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .padding_bottom = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .padding_right = { STYLE_KEYWORD_INHERIT, STYLE_LENGTH_UNIT_FIXED, 0.0f },
    .text_align = { STYLE_KEYWORD_INHERIT, TEXT_ALIGN_MIN },
    .vertical_align = { STYLE_KEYWORD_INHERIT, TEXT_ALIGN_MIN }
};

const Style& GetDefaultStyle()
{
    return g_default_style;
}

static bool DeserializeStyleParameter(Stream* stream, StyleParameter& value)
{
    value.keyword = (StyleKeyword)ReadU8(stream);
    return value.keyword == STYLE_KEYWORD_OVERWRITE;
}

static void DeserializeStyleParameter(Stream* stream, StyleInt& value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter&)value))
        return;
    value.value = ReadI32(stream);
}

static void DeserializeStyleParameter(Stream* stream, StyleColor& value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter&)value))
        return;
    value.value = ReadColor(stream);
}

static void DeserializeStyleParameter(Stream* stream, StyleTextAlign& value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter&)value))
        return;
    value.value = (TextAlign)ReadU8(stream);
}

static void DeserializeStyleParameter(Stream* stream, StyleFlexDirection& value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter&)value))
        return;
    value.value = (FlexDirection)ReadU8(stream);
}

static void DeserializeStyleParameter(Stream* stream, StyleLength& value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter&)value))
        return;

    value.unit = (StyleLengthUnit)ReadU8(stream);
    value.value = ReadFloat(stream);
}

static void DeserializeStyleParameter(Stream* stream, StyleFont& value)
{
    if (!DeserializeStyleParameter(stream, (StyleParameter&)value))
        return;

    ReadString(stream, value.name, MAX_NAME_LENGTH);
    value.id = -1;
}

void DeserializeStyle(Stream* stream, Style& style)
{
    DeserializeStyleParameter(stream, style.flex_direction);
    DeserializeStyleParameter(stream, style.width);
    DeserializeStyleParameter(stream, style.height);
    DeserializeStyleParameter(stream, style.background_color);
    DeserializeStyleParameter(stream, style.color);
    DeserializeStyleParameter(stream, style.font);
    DeserializeStyleParameter(stream, style.font_size);
    DeserializeStyleParameter(stream, style.margin_top);
    DeserializeStyleParameter(stream, style.margin_left);
    DeserializeStyleParameter(stream, style.margin_bottom);
    DeserializeStyleParameter(stream, style.margin_right);
    DeserializeStyleParameter(stream, style.padding_top);
    DeserializeStyleParameter(stream, style.padding_left);
    DeserializeStyleParameter(stream, style.padding_bottom);
    DeserializeStyleParameter(stream, style.padding_right);
    DeserializeStyleParameter(stream, style.text_align);
    DeserializeStyleParameter(stream, style.vertical_align);
}

Style DeserializeStyle(Stream* stream)
{
    Style style = {};
    DeserializeStyle(stream, style);
    return style;
}

static bool SerializeParameter(Stream* stream, const StyleParameter& value)
{
    WriteU8(stream, (u8)value.keyword);
    return value.keyword == STYLE_KEYWORD_OVERWRITE;
}

static void SerializeParameter(Stream* stream, const StyleInt& value)
{
    if (!SerializeParameter(stream, (StyleParameter&)value))
        return;
    WriteI32(stream, value.value);
}

static void SerializeParameter(Stream* stream, const StyleColor& value)
{
    if (!SerializeParameter(stream, (StyleParameter&)value))
        return;
    WriteColor(stream, value.value);
}

static void SerializeParameter(Stream* stream, const StyleFlexDirection& value)
{
    if (!SerializeParameter(stream, (StyleParameter&)value))
        return;
    WriteU8(stream, (uint8_t)value.value);
}

static void SerializeParameter(Stream* stream, const StyleTextAlign& value)
{
    if (!SerializeParameter(stream, (StyleParameter&)value))
        return;
    WriteU8(stream, (uint8_t)value.value);
}

static void SerializeParameter(Stream* stream, const StyleLength& value)
{
    if (!SerializeParameter(stream, (StyleParameter&)value))
        return;

    WriteU8(stream, (uint8_t)value.unit);
    WriteFloat(stream, value.value);
}

static void SerializeParameter(Stream* stream, const StyleFont& value)
{
    if (!SerializeParameter(stream, (StyleParameter&)value))
        return;

    WriteString(stream, value.name);
}

void SerializeStyle(const Style& style, Stream* stream)
{
    SerializeParameter(stream, style.flex_direction);
    SerializeParameter(stream, style.width);
    SerializeParameter(stream, style.height);
    SerializeParameter(stream, style.background_color);
    SerializeParameter(stream, style.color);
    SerializeParameter(stream, style.font);
    SerializeParameter(stream, style.font_size);
    SerializeParameter(stream, style.margin_top);
    SerializeParameter(stream, style.margin_left);
    SerializeParameter(stream, style.margin_bottom);
    SerializeParameter(stream, style.margin_right);
    SerializeParameter(stream, style.padding_top);
    SerializeParameter(stream, style.padding_left);
    SerializeParameter(stream, style.padding_bottom);
    SerializeParameter(stream, style.padding_right);
    SerializeParameter(stream, style.text_align);
    SerializeParameter(stream, style.vertical_align);
}

void MergeStyles(Style& dst, const Style& src, bool apply_defaults)
{
#define STYLE_MERGE(n) if (src.n.parameter.keyword >= dst.n.parameter.keyword || (apply_defaults && dst.n.parameter.keyword != STYLE_KEYWORD_INLINE)) dst.n = src.n
    STYLE_MERGE(flex_direction);
    STYLE_MERGE(color);
    STYLE_MERGE(background_color);
    STYLE_MERGE(width);
    STYLE_MERGE(height);
    STYLE_MERGE(font);
    STYLE_MERGE(font_size);
    STYLE_MERGE(margin_top);
    STYLE_MERGE(margin_left);
    STYLE_MERGE(margin_bottom);
    STYLE_MERGE(margin_right);
    STYLE_MERGE(padding_top);
    STYLE_MERGE(padding_left);
    STYLE_MERGE(padding_bottom);
    STYLE_MERGE(padding_right);
    STYLE_MERGE(text_align);
    STYLE_MERGE(vertical_align);
#undef STYLE_MERGE
}
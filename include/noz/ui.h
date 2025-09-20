//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "input.h"

struct StyleSheet : Asset {};

struct StyleId
{
    u16 style_sheet_id;
    u16 id;
};

enum TextAlign
{
    TEXT_ALIGN_MIN,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_MAX
};

enum StyleKeyword
{
    STYLE_KEYWORD_INHERIT,
    STYLE_KEYWORD_OVERWRITE,
    STYLE_KEYWORD_INLINE
};

enum FlexDirection
{
    FLEX_DIRECTION_ROW,
    FLEX_DIRECTION_COL
} ;

enum StyleLengthUnit
{
    STYLE_LENGTH_UNIT_FIXED,
    STYLE_LENGTH_UNIT_PERCENT,
    STYLE_LENGTH_UNIT_AUTO
};

enum PositionType
{
    POSITION_TYPE_RELATIVE,
    POSITION_TYPE_ABSOLUTE
};

struct StyleParameter
{
    StyleKeyword keyword;
};

struct StyleLength
{
    StyleParameter parameter;
    StyleLengthUnit unit;
    float value;
};

struct StyleColor
{
    StyleParameter parameter;
    Color value;
};

struct StyleFloat
{
    StyleParameter parameter;
    float value;
};

struct StyleInt
{
    StyleParameter parameter;
    int value;
};

struct StyleBool
{
    StyleParameter parameter;
    bool value;
};

struct StyleFlexDirection
{
    StyleParameter parameter;
    FlexDirection value;
};

struct StyleTextAlign
{
    StyleParameter parameter;
    TextAlign value;
};

struct StyleFont
{
    StyleParameter parameter;
    int id;
    char name[MAX_NAME_LENGTH];
};

struct StylePosition
{
    StyleParameter parameter;
    PositionType value;
};

struct Style
{
    StyleFlexDirection flex_direction;
    StylePosition position;
    StyleLength width;
    StyleLength height;
    StyleColor background_color;
    StyleColor background_vignette_color;
    StyleFloat background_vignette_intensity;
    StyleFloat background_vignette_smoothness;
    StyleColor color;
    StyleFont font;
    StyleInt font_size;
    StyleLength margin_top;
    StyleLength margin_left;
    StyleLength margin_bottom;
    StyleLength margin_right;
    StyleLength padding_top;
    StyleLength padding_left;
    StyleLength padding_bottom;
    StyleLength padding_right;
    StyleTextAlign text_align;
    StyleTextAlign vertical_align;
    StyleFloat rotate;
    StyleFloat translate_x;
    StyleFloat translate_y;
    StyleFloat scale;
    StyleFloat translate_origin_x;
    StyleFloat translate_origin_y;
};

struct ElementInput
{
    InputCode button;
    Vec2 mouse_position;
    Rect bounds;
    void* user_data;
};

constexpr StyleId STYLE_DEFAULT = { 0xFFFF, 0xFFFF };

typedef bool (*ElementInputFunc)(const ElementInput& input);

// @style
extern const Style& GetDefaultStyle();
extern void DeserializeStyle(Stream* stream, Style& style);
extern Style DeserializeStyle(Stream* stream);
extern void SerializeStyle(const Style& style, Stream* stream);
extern void MergeStyles(Style& dst, const Style& src, bool apply_defaults=false);
extern const Style& GetStyle(const StyleId& style_id);

inline bool IsAuto(const StyleLength& length) { return length.unit == STYLE_LENGTH_UNIT_AUTO; }
inline bool IsFixed(const StyleLength& length) { return length.unit == STYLE_LENGTH_UNIT_FIXED; }
inline bool IsPercent(const StyleLength& length) { return length.unit == STYLE_LENGTH_UNIT_PERCENT; }

// @ui
extern void BeginUI(u32 ref_width, u32 ref_height);
extern void DrawUI();
extern void EndUI();
extern void BeginCanvas(const StyleId& style_id = STYLE_DEFAULT);
extern void BeginWorldCanvas(Camera* camera, const Vec2& position, const Vec2& size, const StyleId& style_id = STYLE_DEFAULT);
extern void EndCanvas();
extern void BeginElement(const StyleId& style_id = STYLE_DEFAULT);
extern void EndElement();
extern void SetInputHandler(ElementInputFunc func, void* user_data = nullptr);
extern bool IsMouseOverElement();
extern void SetElementStyle(const StyleId& style_id);
extern bool DidMouseEnterElement();
extern bool DidMouseLeaveElement();

// @elements
extern void EmptyElement(const StyleId& style_id = STYLE_DEFAULT);
extern void Label(const char* text, const StyleId& style_id = STYLE_DEFAULT);
extern void Image(Material* material, const StyleId& style_id = STYLE_DEFAULT);
extern void MeshElement(Mesh* mesh, Material* material, const StyleId& style_id = STYLE_DEFAULT);

extern StyleSheet** STYLESHEET;

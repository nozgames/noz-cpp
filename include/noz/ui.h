//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "input.h"

// @types
struct StyleSheet : Asset {};

// @style

typedef u32 PseudoState;

constexpr PseudoState PSEUDO_STATE_NONE     = 0;
constexpr PseudoState PSEUDO_STATE_HOVER    = 1 << 0;
constexpr PseudoState PSEUDO_STATE_ACTIVE   = 1 << 1;
constexpr PseudoState PSEUDO_STATE_SELECTED = 1 << 2;
constexpr PseudoState PSEUDO_STATE_DISABLED = 1 << 3;
constexpr PseudoState PSEUDO_STATE_FOCUSED  = 1 << 4;
constexpr PseudoState PSEUDO_STATE_PRESSED  = 1 << 5;
constexpr PseudoState PSEUDO_STATE_CHECKED  = 1 << 6;

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

struct Style
{
    StyleFlexDirection flex_direction;
    StyleLength width;
    StyleLength height;
    StyleColor background_color;
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
};

struct ElementInput
{
    InputCode button;
    Vec2 mouse_position;
    Rect bounds;
    void* user_data;
};

typedef bool (*ElementInputFunc)(const ElementInput& input);

const Style& GetDefaultStyle();
void DeserializeStyle(Stream* stream, Style& style);
Style DeserializeStyle(Stream* stream);
void SerializeStyle(const Style& style, Stream* stream);
void MergeStyles(Style& dst, const Style& src, bool apply_defaults=false);

// @stylesheet
const Style& GetStyle(StyleSheet* sheet, const Name* name, PseudoState pseudo_state);
bool GetStyle(StyleSheet* sheet, const Name* id, PseudoState pseudo_state, Style* result);
bool HasStyle(StyleSheet* sheet, const Name* name, PseudoState pseudo_state);

// @style_length
inline bool IsAuto(const StyleLength& length) { return length.unit == STYLE_LENGTH_UNIT_AUTO; }
inline bool IsFixed(const StyleLength& length) { return length.unit == STYLE_LENGTH_UNIT_FIXED; }
inline bool IsPercent(const StyleLength& length) { return length.unit == STYLE_LENGTH_UNIT_PERCENT; }

// @ui
extern void BeginUI(u32 ref_width, u32 ref_height);
extern void EndUI();
extern void BeginCanvas(const Name* name = nullptr, StyleSheet* styles = nullptr);
extern void BeginWorldCanvas(Camera* camera, const Vec2& position, const Vec2& size, const Name* name=nullptr, StyleSheet* styles=nullptr);
extern void EmptyElement(const Name* id);
extern void BeginElement(const Name* id);
extern void EndElement();
extern void EndCanvas();
extern void PushStyles(StyleSheet* sheet);
extern void PopStyles();
extern void DrawUI();
extern void Label(const char* text, const Name* id);
extern void Image(Material* material, const Name* id);;
extern void SetInputHandler(ElementInputFunc func, void* user_data = nullptr);

extern StyleSheet** STYLESHEET;
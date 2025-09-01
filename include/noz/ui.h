//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @types
struct StyleSheet : Object {};

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
    color_t value;
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

struct Style
{
    StyleFlexDirection flex_direction;
    StyleLength width;
    StyleLength height;
    StyleColor background_color;
    StyleColor color;
    StyleInt font_size;
    StyleLength margin_top;
    StyleLength margin_left;
    StyleLength margin_bottom;
    StyleLength margin_right;
    StyleLength padding_top;
    StyleLength padding_left;
    StyleLength padding_bottom;
    StyleLength padding_right;
};

const Style& GetDefaultStyle();
void DeserializeStyle(Stream* stream, Style& style);
Style DeserializeStyle(Stream* stream);
void SerializeStyle(const Style& style, Stream* stream);
void MergeStyles(Style& dst, const Style& src, bool apply_defaults=false);

// @stylesheet
const Style& GetStyle(StyleSheet* sheet, const name_t* name, PseudoState pseudo_state);
bool GetStyle(StyleSheet* sheet, const name_t* id, PseudoState pseudo_state, Style* result);
bool HasStyle(StyleSheet* sheet, const name_t* name, PseudoState pseudo_state);
const name_t* GetName(StyleSheet* sheet);

// @style_length
inline bool IsAuto(const StyleLength& length) { return length.unit == STYLE_LENGTH_UNIT_AUTO; }
inline bool IsFixed(const StyleLength& length) { return length.unit == STYLE_LENGTH_UNIT_FIXED; }
inline bool IsPercent(const StyleLength& length) { return length.unit == STYLE_LENGTH_UNIT_PERCENT; }

// @ui
extern void SetDefaultFont(Font* font);
extern Font* GetDefaultFont();
extern void BeginUI();
extern void EndUI();
extern void BeginCanvas(u32 referenceWidth, u32 referenceHeight);
extern void BeginElement(const name_t* id);
extern void EndElement();
extern void EndCanvas();
extern void SetStyleSheet(StyleSheet* sheet);
extern void DrawUI();
extern void Label(const char* text, const name_t* id);

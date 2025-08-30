//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @types
struct StyleSheet : Object {};
struct Canvas : Entity {};
struct Element : Object {};
struct Label : Element {};

#define ELEMENT_BASE_SIZE 360
#define ELEMENT_BASE ElementBase __element;

struct ElementBase { u8 __entity[ELEMENT_BASE_SIZE]; };

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
    FLEX_DIRECTION_COL,
    FLEX_DIRECTION_ROW_REVERSE,
    FLEX_DIRECTION_COL_REVERSE
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

// @element_traits
struct ElementTraits
{
    vec2(*measure_content)(Element*, const vec2& available_size, const Style& style) = nullptr;
    void(*render_content)(Element*, const Style& style) = nullptr;
    void(*on_apply_style)(Element*, const Style&) = nullptr;
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
void SetDefaultFont(Font* font);
Font* GetDefaultFont();

// @canvas
enum CanvasType
{
    CANVAS_TYPE_SCREEN,
    CANVAS_TYPE_WORLD,
};

Canvas* CreateCanvas(Allocator* allocator, CanvasType type, float reference_width, float reference_height, const name_t* id = nullptr);
StyleSheet* GetStyleSheet(Canvas* canvas);
Element* GetRootElement(Canvas* canvas);
void SetStyleSheet(Canvas* canvas, StyleSheet* sheet);
StyleSheet* GetStyleSheet(Canvas* canvas);
void MarkDirty(Canvas* canvas);
void SetVisible(Canvas* element, bool visible);

// @element

extern const ElementTraits* g_element_traits[];

void SetElementTraits(type_t id, const ElementTraits* traits);
inline const ElementTraits* GetElementTraits(type_t id) { return g_element_traits[id]; }
inline const ElementTraits* GetElementTraits(Element* element) { return g_element_traits[GetType(element)]; }

Element* CreateElement(Allocator* allocator, size_t element_size, type_t element_type, const name_t* id = nullptr);
Element* CreateElement(Allocator* allocator, const name_t* id = nullptr);
void SetParent(Element* element, Element* parent);
void SetParent(Element* element, Canvas* parent);
void AddChild(Element* element, Element* child);
void RemoveChild(Element* element, Element* child);
void RemoveFromParent(Element* element);
bool IsVisible(Element* element);
void SetVisible(Element* element, bool visible);
void SetPseudoState(Element* element, PseudoState state, bool value);
void MarkDirty(Element* element);

// @label
Label* CreateLabel(Allocator* allocator, const char* text, const name_t* id);
Label* CreateLabel(Allocator* allocator, const text_t& text, const name_t* id);
Label* CreateLabel(Allocator* allocator, const name_t* id);
const text_t& GetText(Label* label);
void SetText(Label* label, const char* text);
void SetText(Label* label, const text_t& text);

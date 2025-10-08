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
    StyleFloat transform_origin_x;
    StyleFloat transform_origin_y;
    StyleColor border_color;
    StyleFloat border_width;
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
extern void SetElementTransform(const Vec2& translate, float rotate, float scale);
extern void SetElementTranslate(const Vec2& translate);
extern void SetElementRotate(float rotate);
extern void SetElementScale(float scale);

// @elements
extern void EmptyElement(const StyleId& style_id = STYLE_DEFAULT);
extern void Label(const char* text, const StyleId& style_id = STYLE_DEFAULT);
extern void Image(Material* material, const StyleId& style_id = STYLE_DEFAULT);
extern void MeshElement(Mesh* mesh, Material* material, const StyleId& style_id = STYLE_DEFAULT);

extern StyleSheet** STYLESHEET;

typedef u32 ElementState;
constexpr ElementState ELEMENT_STATE_NONE        = 0;
constexpr ElementState ELEMENT_STATE_HOVERED     = 1 << 0;
constexpr ElementState ELEMENT_STATE_PRESSED     = 1 << 1;

typedef Color (*AnimatedColorFunc)(ElementState state, float time);

struct EdgeInsets {
    float top = 0.0f;
    float left = 0.0f;
    float bottom = 0.0f;
    float right = 0.0f;

    EdgeInsets() = default;

    EdgeInsets(float t, float l, float b, float r)
        : top(t), left(l), bottom(b), right(r) {}

    EdgeInsets(float v) : top(v), left(v), bottom(v), right(v) {}
};

struct RowStyle {
    float spacing = 0.0f;
};

struct TransformStyle {
    Vec2 origin = {0.5f, 0.5f};
    Vec2 translate = VEC2_ZERO;
    float rotate = 0.0f;
    Vec2 scale = VEC2_ONE;
};

struct ColumnStyle {
    float spacing = 0.0f;
};

struct ExpandedStyle {
    float flex = 1.0f;
};

struct GestureDetectorStyle {
    void (*on_tap)() = nullptr;
    void (*on_tap_down)() = nullptr;
    void (*on_tap_up)() = nullptr;
};

struct LabelStyle {
    Font* font = nullptr;
    int font_size = 16;
    Color color = COLOR_WHITE;
    TextAlign align = TEXT_ALIGN_MIN;
    TextAlign vertical_align = TEXT_ALIGN_MIN;
};

struct MouseRegionStyle {
    // todo: cursor
    void (*on_enter)() = nullptr;
    void (*on_exit)() = nullptr;
    void (*on_hover)() = nullptr;
};

struct ImageStyle {
    Color color = COLOR_WHITE;
};

struct BorderStyle {
    float width = 1.0f;
    Color color = COLOR_WHITE;
};

struct RectangleStyle {
    Color color = COLOR_WHITE;
    AnimatedColorFunc color_func = nullptr;
};

struct CanvasStyle {
    Color color;
};

struct ContainerStyle {
    float width = F32_MAX;
    float height = F32_MAX;
    EdgeInsets margin;
    EdgeInsets padding;
    Color color;
    BorderStyle border;
};


// @layout
extern void Canvas(const CanvasStyle& style, void (*children)());
extern void Stack(void (*children)() = nullptr);
extern void Container(const ContainerStyle& style, void (*children)() = nullptr);
extern void Column(const ColumnStyle& style, void (*children)() = nullptr);
extern void Row(const RowStyle& style, void (*children)() = nullptr);
extern void Border(const BorderStyle& style, void (*children)() = nullptr);
extern void Inset(const EdgeInsets& insets, void (*children)() = nullptr);
extern void Inset(float amount, void (*children)() = nullptr);
extern void SizedBox(int width, int height, void (*children)() = nullptr);

// @modifiers
extern void Transformed(const TransformStyle& style, void (*children)() = nullptr);
extern void Expanded(const ExpandedStyle& style, void (*children)() = nullptr);

// @input
extern void GestureDetector(const GestureDetectorStyle& style, void (*children)() = nullptr);
extern void MouseRegion(const MouseRegionStyle& style, void (*children)() = nullptr);

// @drawing
extern void Label(const char* text, const LabelStyle& style = {});
extern void Image(Material* material, const ImageStyle& style = {});
extern void Image(Material* material, Mesh* mesh, const ImageStyle& style = {});
extern void Rectangle(const RectangleStyle& style = {});

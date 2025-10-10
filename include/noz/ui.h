//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "input.h"

enum TextAlign
{
    TEXT_ALIGN_MIN,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_MAX
};

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
    void (*on_tap)(void* user_data) = nullptr;
    void (*on_tap_down)(void* user_data) = nullptr;
    void (*on_tap_up)(void* user_data) = nullptr;
    void* user_data;
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

// @frame
extern void BeginUI(u32 ref_width, u32 ref_height);
extern void DrawUI();
extern void EndUI();

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

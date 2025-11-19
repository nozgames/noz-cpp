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

typedef Color (*AnimatedColorFunc)(ElementState state, float time, void* user_data);

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

enum CanvasType {
    CANVAS_TYPE_SCREEN,
    CANVAS_TYPE_WORLD
};

struct Alignment {
    float x; // -1.0 (min) to 1.0 (max)
    float y; // -1.0 (min) to 1.0 (max)
};

constexpr Alignment ALIGNMENT_TOP           = {  F32_MAX, -1.0f };
constexpr Alignment ALIGNMENT_TOP_LEFT      = { -1.0f, -1.0f };
constexpr Alignment ALIGNMENT_TOP_CENTER    = {  0.0f, -1.0f };
constexpr Alignment ALIGNMENT_TOP_RIGHT     = {  1.0f, -1.0f };
constexpr Alignment ALIGNMENT_CENTER        = {  F32_MAX, 0.0f };
constexpr Alignment ALIGNMENT_CENTER_CENTER = {  0.0f,  0.0f };
constexpr Alignment ALIGNMENT_CENTER_LEFT   = { -1.0f,  0.0f };
constexpr Alignment ALIGNMENT_CENTER_RIGHT  = {  1.0f,  0.0f };
constexpr Alignment ALIGNMENT_BOTTOM        = {  F32_MAX, 1.0f };
constexpr Alignment ALIGNMENT_BOTTOM_LEFT   = { -1.0f,  1.0f };
constexpr Alignment ALIGNMENT_BOTTOM_RIGHT  = {  1.0f,  1.0f };
constexpr Alignment ALIGNMENT_BOTTOM_CENTER = {  0.0f,  1.0f };

struct AlignStyle {
    Alignment alignment;
    EdgeInsets margin;
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

struct TapDetails {
    Vec2 position;
    void* user_data;
    int id;
};

struct DragDetails {
    void* user_data;
    int id;
};

struct GestureDetectorStyle {
    void (*on_tap)(const TapDetails& details);
    void (*on_tap_down)(const TapDetails& details);
    void (*on_tap_up)(const TapDetails& details);
    void (*on_drag)(const DragDetails& details);
    void* user_data;
    int id;
};

struct LabelStyle {
    Font* font = nullptr;
    int font_size = 16;
    Color color = COLOR_WHITE;
    Alignment align = ALIGNMENT_TOP_LEFT;
    Material* material;
};

struct MouseRegionStyle {
    // todo: cursor
    void (*on_enter)(void*) = nullptr;
    void (*on_exit)(void*) = nullptr;
    void (*on_hover)(void*) = nullptr;
    void* user_data;
};

struct ImageStyle {
    Color color = COLOR_WHITE;
    AnimatedColorFunc color_func = nullptr;
    void* color_func_user_data = nullptr;
};

struct BorderStyle {
    float width = 0.0f;
    Color color = COLOR_WHITE;
};

struct RectangleStyle {
    Color color = COLOR_WHITE;
    AnimatedColorFunc color_func = nullptr;
    void* color_func_user_data = nullptr;
};

struct SizedBoxStyle {
    float width = F32_MAX;
    float height = F32_MAX;
};

struct CanvasStyle {
    CanvasType type = CANVAS_TYPE_SCREEN;
    Color color;
    Camera* world_camera;
    Vec2 world_position;
    Vec2 world_size;
};

struct ContainerStyle {
    float width = F32_MAX;
    float height = F32_MAX;
    EdgeInsets margin;
    EdgeInsets padding;
    Color color;
    BorderStyle border;
    void* user_data;
};

// @frame
extern void BeginUI(u32 ref_width, u32 ref_height);
extern void DrawUI();
extern void EndUI();

// @layout
extern void Align(const AlignStyle& style, const std::function<void()>& children = nullptr);
extern void Canvas(const CanvasStyle& style, const std::function<void()>& children = nullptr);
inline void Canvas(const std::function<void()>& children = nullptr) { Canvas({}, children); }
extern void Stack(void (*children)() = nullptr);
extern void Container(const ContainerStyle& style, const std::function<void()>& children=nullptr);
extern void Column(const ColumnStyle& style, const std::function<void()>& children = nullptr);
inline void Column(void (*children)() = nullptr) { Column({}, children); }
extern void Row(const RowStyle& style, const std::function<void()>& children = nullptr);
inline void Row(const std::function<void()>& children = nullptr) { Row({}, children); }
extern void Border(const BorderStyle& style, const std::function<void()>& children = nullptr);
extern void Inset(const EdgeInsets& insets, void (*children)() = nullptr);
extern void Inset(float amount, void (*children)() = nullptr);
extern void SizedBox(const SizedBoxStyle& style, const std::function<void()>& children = nullptr);
extern void Center(const std::function<void()>& children);

// @modifiers
extern void Transformed(const TransformStyle& style, const std::function<void()>& children = nullptr);
extern void Expanded(const ExpandedStyle& style, const std::function<void()>& children = nullptr);
inline void Expanded(const std::function<void()>& children = nullptr) { Expanded({}, children); }

// @input
extern void GestureBlocker(const std::function<void()>& children);
extern void GestureDetector(const GestureDetectorStyle& style, const std::function<void()>& children = nullptr);
extern void MouseRegion(const MouseRegionStyle& style, const std::function<void()>& children = nullptr);

// @drawing
extern void Label(const char* text, const LabelStyle& style = {});
extern void Image(Material* material, const ImageStyle& style = {});
extern void Image(Material* material, Mesh* mesh, const ImageStyle& style = {});
inline void Image(Mesh* mesh, const ImageStyle& style = {}) { Image(nullptr, mesh, style); }
extern void Rectangle(const RectangleStyle& style = {});

// @edgeinsets
inline EdgeInsets EdgeInsetsAll(float v) { return EdgeInsets(v, v, v, v); }
inline EdgeInsets EdgeInsetsTop(float v) { return EdgeInsets(v, 0, 0, 0); }
inline EdgeInsets EdgeInsetsTopLeft(float t, float l) { return EdgeInsets(t, l, 0, 0); }
inline EdgeInsets EdgeInsetsTopLeft(float v) { return EdgeInsets(v, v, 0, 0); }
inline EdgeInsets EdgeInsetsTopRight(float v) { return EdgeInsets(v, 0, 0, v); }
inline EdgeInsets EdgeInsetsBottom(float v) { return EdgeInsets(0, 0, v, 0); }
inline EdgeInsets EdgeInsetsBottomLeft(float b, float l) { return EdgeInsets(0, l, b, 0); }
inline EdgeInsets EdgeInsetsBottomLeft(float v) { return EdgeInsets(0, v, v, 0); }
inline EdgeInsets EdgeInsetsBottomRight(float b, float r) { return EdgeInsets(0, 0, b, r); }
inline EdgeInsets EdgeInsetsBottomRight(float v) { return EdgeInsets(0, 0, v, v); }
inline EdgeInsets EdgeInsetsRight(float v) { return EdgeInsets(0,0,0,v); }
inline EdgeInsets EdgeInsetsLeft(float v) { return EdgeInsets(0,v,0,0); }
inline EdgeInsets EdgeInsetsLeftRight(float v) { return EdgeInsets(0,v,0,v); }


// @text_engine
struct TextMesh {};

struct TextRequest {
    text_t text;
    Font* font;
    int font_size;
};

TextMesh* CreateTextMesh(Allocator* allocator, const TextRequest& request);
Vec2 MeasureText(const text_t& text, Font* font, float font_size);
Mesh* GetMesh(TextMesh* tm);
Material* GetMaterial(TextMesh* tm);
Vec2 GetSize(TextMesh* tm);

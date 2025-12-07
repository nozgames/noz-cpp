//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

#include "input.h"

constexpr float F32_AUTO = F32_MAX;

typedef u32 ElementFlags;
constexpr ElementFlags ELEMENT_FLAG_NONE        = 0;
constexpr ElementFlags ELEMENT_FLAG_HOVERED     = 1 << 0;
constexpr ElementFlags ELEMENT_FLAG_PRESSED     = 1 << 1;
constexpr ElementFlags ELEMENT_FLAG_DOWN        = 1 << 2;

typedef Color (*AnimatedColorFunc)(ElementFlags state, float time, void* user_data);

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

enum Align {
    ALIGN_NONE,
    ALIGN_TOP,
    ALIGN_LEFT,
    ALIGN_BOTTOM,
    ALIGN_RIGHT,
    ALIGN_TOP_LEFT,
    ALIGN_TOP_RIGHT,
    ALIGN_TOP_CENTER,
    ALIGN_CENTER_LEFT,
    ALIGN_CENTER_RIGHT,
    ALIGN_CENTER,
    ALIGN_BOTTOM_LEFT,
    ALIGN_BOTTOM_RIGHT,
    ALIGN_BOTTOM_CENTER,

    ALIGN_COUNT
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

struct SceneStyle {
    Camera* camera;
    void* user_data;
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
    Align align = ALIGN_TOP_LEFT;
    Material* material;
};

struct MouseRegionStyle {
    // todo: cursor
    void (*on_enter)(void*) = nullptr;
    void (*on_exit)(void*) = nullptr;
    void (*on_hover)(void*) = nullptr;
    void* user_data;
};

enum ImageStretch {
    IMAGE_STRETCH_NONE,
    IMAGE_STRETCH_FILL,
    IMAGE_STRETCH_UNIFORM,
    IMAGE_STRETCH_UNIFORM_FILL
};

struct ImageStyle {
    Color color = COLOR_WHITE;
    Vec2Int color_offset;
    float scale = 1.0f;
    Vec2 uv = VEC2_ZERO;
    Vec2 st = VEC2_ONE;
    ImageStretch stretch = IMAGE_STRETCH_UNIFORM;
    Material* material;
};

struct BorderStyle {
    float width = 0.0f;
    Color color = COLOR_WHITE;
};

struct RectangleStyle {
    float width = F32_AUTO;
    float height = F32_AUTO;
    Color color = COLOR_WHITE;
    Vec2Int color_offset;
};

struct CanvasStyle {
    CanvasType type = CANVAS_TYPE_SCREEN;
    Color color;
    Camera* world_camera;
    Vec2 world_position;
    Vec2 world_size;
};

struct ContainerStyle {
    float width = F32_AUTO;
    float height = F32_AUTO;
    Align align = ALIGN_TOP_LEFT;
    EdgeInsets margin;
    EdgeInsets padding;
    Color color;
    Vec2Int color_offset;
    BorderStyle border;
    void* user_data;
};

// @common
extern void BeginUI(u32 ref_width, u32 ref_height);
extern void DrawUI();
extern void EndUI();
extern Vec2 ScreenToUI(const Vec2& screen_pos);
extern bool CheckElementFlags(ElementFlags flags);
extern u64 GetElementId();
extern Vec2 ScreenToElement(const Vec2& screen);

// @layout
extern void BeginCanvas(const CanvasStyle& style={});
extern void BeginContainer(const ContainerStyle& style={});
extern void BeginColumn(const ColumnStyle& style={});
extern void BeginRow(const RowStyle& style={});
extern void BeginTransformed(const TransformStyle& style);
extern void BeginBorder(const BorderStyle& style);
extern void BeginCenter();
extern void BeginExpanded(const ExpandedStyle& style={});
extern void EndCanvas();
extern void EndContainer();
extern void EndColumn();
extern void EndRow();
extern void EndBorder();
extern void EndCenter();
extern void EndExpanded();
extern void EndTransformed();
extern void Container(const ContainerStyle& style);
extern void Expanded(const ExpandedStyle& style={});
extern void Spacer(float size);

// @input
inline bool IsHovered() { return CheckElementFlags(ELEMENT_FLAG_HOVERED); }
inline bool WasPressed() { return CheckElementFlags(ELEMENT_FLAG_PRESSED); }
inline bool IsDown() { return CheckElementFlags(ELEMENT_FLAG_DOWN); }

// @drawing
extern void Label(const char* text, const LabelStyle& style = {});
inline void Label(const Text& text, const LabelStyle& style = {}) {
    Label(text.value, style);
}
inline void Label(const Name* name, const LabelStyle& style = {}) {
    Label(name->value, style);
}
extern void Image(Mesh* mesh, const ImageStyle& style = {});
extern void Image(AnimatedMesh* mesh, float time, const ImageStyle& style = {});
extern void Rectangle(const RectangleStyle& style = {});
extern void Scene(const SceneStyle& style, void (*draw_scene)(void*) = nullptr);

// @edgeinsets
inline EdgeInsets EdgeInsetsAll(float v) { return EdgeInsets(v, v, v, v); }
inline EdgeInsets EdgeInsetsTop(float v) { return EdgeInsets(v, 0, 0, 0); }
inline EdgeInsets EdgeInsetsTopLeft(float t, float l) { return EdgeInsets(t, l, 0, 0); }
inline EdgeInsets EdgeInsetsTopLeft(float v) { return EdgeInsets(v, v, 0, 0); }
inline EdgeInsets EdgeInsetsTopRight(float v) { return EdgeInsets(v, 0, 0, v); }
inline EdgeInsets EdgeInsetsTopRight(float t, float l) { return EdgeInsets(t, 0, 0, l); }
inline EdgeInsets EdgeInsetsBottom(float v) { return EdgeInsets(0, 0, v, 0); }
inline EdgeInsets EdgeInsetsBottomLeft(float b, float l) { return EdgeInsets(0, l, b, 0); }
inline EdgeInsets EdgeInsetsBottomLeft(float v) { return EdgeInsets(0, v, v, 0); }
inline EdgeInsets EdgeInsetsBottomRight(float b, float r) { return EdgeInsets(0, 0, b, r); }
inline EdgeInsets EdgeInsetsBottomRight(float v) { return EdgeInsets(0, 0, v, v); }
inline EdgeInsets EdgeInsetsRight(float v) { return EdgeInsets(0,0,0,v); }
inline EdgeInsets EdgeInsetsLeft(float v) { return EdgeInsets(0,v,0,0); }
inline EdgeInsets EdgeInsetsLeftRight(float v) { return EdgeInsets(0,v,0,v); }
inline EdgeInsets EdgeInsetsLeftRight(float l, float r) { return EdgeInsets(0,l,0,r); }

// @text_engine
struct TextMesh {};

struct TextRequest {
    Text text;
    Font* font;
    int font_size;
};

extern TextMesh* CreateTextMesh(Allocator* allocator, const TextRequest& request);
extern Vec2 MeasureText(const Text& text, Font* font, float font_size);
extern Bounds2 MeasureText(const Text& text, Font* font, float font_size, int start, int end);
extern Mesh* GetMesh(TextMesh* tm);
extern Material* GetMaterial(TextMesh* tm);
extern Vec2 GetSize(TextMesh* tm);

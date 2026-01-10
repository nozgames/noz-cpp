//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

#include "input.h"

namespace noz {

    constexpr float F32_AUTO = F32_MAX;

    typedef u32 ElementFlags;
    constexpr ElementFlags ELEMENT_FLAG_NONE        = 0;
    constexpr ElementFlags ELEMENT_FLAG_HOVERED     = 1 << 0;
    constexpr ElementFlags ELEMENT_FLAG_PRESSED     = 1 << 1;
    constexpr ElementFlags ELEMENT_FLAG_DOWN        = 1 << 2;

    typedef u8 ElementId;
    constexpr ElementId ELEMENT_ID_NONE = 0;
    constexpr ElementId ELEMENT_ID_MIN = 1;
    constexpr ElementId ELEMENT_ID_MAX = 255;

    typedef u32 CanvasId;
    constexpr CanvasId CANVAS_ID_NONE = 0;
    constexpr CanvasId CANVAS_ID_DEBUG = 1;
    constexpr CanvasId CANVAS_ID_MIN = 2;


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

    enum CanvasType : u8 {
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

    struct TransformStyle {
        Vec2 origin = {0.5f, 0.5f};
        Vec2 translate = VEC2_ZERO;
        float rotate = 0.0f;
        Vec2 scale = VEC2_ONE;
    };

    struct ExpandedStyle {
        float flex = 1.0f;
    };

    struct SceneStyle {
        Camera* camera;
        void* user_data;
        Align align = ALIGN_CENTER;
    };

    struct LabelStyle {
        Font* font = nullptr;
        int font_size = 16;
        Color color = COLOR_WHITE;
        Align align = ALIGN_NONE;
        Material* material;
    };

    enum ImageStretch : u8 {
        IMAGE_STRETCH_NONE,
        IMAGE_STRETCH_FILL,
        IMAGE_STRETCH_UNIFORM
    };

    struct ImageStyle {
        ImageStretch stretch = IMAGE_STRETCH_UNIFORM;
        Align align = ALIGN_NONE;
        float scale = 1.0f;
        Color color = COLOR_WHITE;
        Vec2Int color_offset;
        Vec2 uv = VEC2_ZERO;
        Vec2 st = VEC2_ONE;
        Material* material;
    };

    struct BorderStyle {
        float radius = 0.0f;
        float width = 0.0f;
        Color color = COLOR_TRANSPARENT;
    };

    struct RectangleStyle {
        float width = F32_AUTO;
        float height = F32_AUTO;
        Color color = COLOR_WHITE;
        Vec2Int color_offset;
    };

    struct NavigationStyle {
        ElementId next;
        ElementId prev;
        ElementId left;
        ElementId right;
    };

    struct TextBoxStyle {
        float height = 28.0f;
        float padding = 4.0f;
        Font* font = nullptr;
        int font_size = 16;
        const char* placeholder = nullptr;
        Color background_color = Color8ToColor(55);
        Color text_color = COLOR_WHITE;
        Color placeholder_color = Color8ToColor(100);
        BorderStyle border;
        BorderStyle focus_border;
        bool password = false;
        ElementId id;
        NavigationStyle nav;
    };

    struct PopupStyle {
        Align anchor = ALIGN_TOP_LEFT;
        Align align = ALIGN_TOP_LEFT;
        EdgeInsets margin = {};
    };

    struct CanvasStyle {
        CanvasType type = CANVAS_TYPE_SCREEN;
        Color color;
        Vec2Int color_offset;
        Camera* world_camera;
        Vec2 world_position;
        Vec2 world_size;
        int id;
    };

    struct ContainerStyle {
        float width = F32_AUTO;
        float min_width = 0.0f;
        float height = F32_AUTO;
        float min_height = 0.0f;
        Align align = ALIGN_NONE;
        EdgeInsets margin;
        EdgeInsets padding;
        Color color;
        Vec2Int color_offset;
        BorderStyle border;
        void* user_data;
        float spacing = 0.0f;
        ElementId id;
        NavigationStyle nav;
        bool clip = false;  // Clip children to container bounds (uses stencil buffer)
    };

    typedef void(*VirtualCellRangeFunc)(int virtual_start, int virtual_end);
    typedef void(*VirtualCellFunc)(int cell_index, int virtual_index);

    struct GridStyle {
        float spacing = 0.0f;
        int columns = 3;
        struct {
            float width;
            float height;
        } cell;
        int virtual_count = 0;
        ElementId scroll_id = ELEMENT_ID_NONE;
        VirtualCellFunc virtual_cell_func;
        VirtualCellRangeFunc virtual_range_func;
    };

    struct ScrollableStyle {
        ElementId id = ELEMENT_ID_NONE;
    };

    // @common
    extern void BeginUI(u32 ref_width, u32 ref_height);
    extern void DrawUI();
    extern void EndUI();
    extern Vec2 ScreenToUI(const Vec2& screen_pos);
    extern bool CheckElementFlags(ElementFlags flags);
    extern ElementId GetElementId();
    extern Vec2 ScreenToElement(const Vec2& screen);
    extern Vec2 GetUISize();

    // @layout
    extern void BeginCanvas(const CanvasStyle& style={});
    extern void BeginContainer(const ContainerStyle& style={});
    extern void BeginColumn(const ContainerStyle& style={});
    extern void BeginRow(const ContainerStyle& style={});
    extern void BeginTransformed(const TransformStyle& style);
    extern void BeginBorder(const BorderStyle& style);
    extern void BeginCenter();
    extern void BeginExpanded(const ExpandedStyle& style={});
    extern void BeginGrid(const GridStyle& style);
    extern float BeginScrollable(float offset, const ScrollableStyle& style={});
    extern void BeginPopup(const PopupStyle& style);

    extern void EndCanvas();
    extern void EndContainer();
    extern void EndColumn();
    extern void EndRow();
    extern void EndBorder();
    extern void EndCenter();
    extern void EndExpanded();
    extern void EndGrid();
    extern void EndScrollable();
    extern float GetScrollOffset(ElementId id);
    extern void EndTransformed();
    extern void EndPopup();

    extern void Container(const ContainerStyle& style);
    extern void Expanded(const ExpandedStyle& style={});
    extern void Spacer(float size);

    // @input
    extern bool IsClosed();
    inline bool IsHovered() { return CheckElementFlags(ELEMENT_FLAG_HOVERED); }
    inline bool WasPressed() { return CheckElementFlags(ELEMENT_FLAG_PRESSED); }
    inline bool IsDown() { return CheckElementFlags(ELEMENT_FLAG_DOWN); }
    extern bool HasFocus();
    extern void SetFocus(CanvasId canvas_id, ElementId element_id);
    extern void PushFocus();
    extern void PopFocus();
    extern CanvasId GetFocusedCanvasId();
    extern ElementId GetFocusedElementId();
    extern noz::Rect GetElementRect(ElementId id);

    // @textbox
    extern bool TextBox(Text& text, const TextBoxStyle& style = {});

    // @drawing
    extern void Label(const char* text, const LabelStyle& style = {});
    inline void Label(const Text& text, const LabelStyle& style = {}) {
        Label(text.value, style);
    }
    inline void Label(const Name* name, const LabelStyle& style = {}) {
        Label(name->value, style);
    }
    extern void Image(Mesh* mesh, const ImageStyle& style = {});
    extern void Image(Mesh* mesh, float time, const ImageStyle& style = {});
    extern void Image(Texture* texture, const ImageStyle& style = {});
    extern void Rectangle(const RectangleStyle& style = {});
    extern void Scene(const SceneStyle& style, void (*draw_scene)(void*) = nullptr);

    // @edgeinsets
    inline EdgeInsets EdgeInsetsAll(float v) { return EdgeInsets(v, v, v, v); }
    inline EdgeInsets EdgeInsetsTop(float v) { return EdgeInsets(v, 0, 0, 0); }
    inline EdgeInsets EdgeInsetsTopBottom(float v) { return EdgeInsets(v, 0, v, 0); }
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
    inline EdgeInsets EdgeInsetsSymmetric(float vertical, float horizontal) { return EdgeInsets(vertical, horizontal, vertical, horizontal); }

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
}

//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

enum StyleId {
    STYLE_ID_DARK,
    STYLE_ID_COUNT
};

struct Style {
    // @general
    Color background_color;
    Color selection_color;

    // @workspace
    Color workspace_color;
    Color grid_color;
    Color overlay_background_color;
    Color overlay_text_color;
    Color overlay_accent_text_color;
    Color overlay_icon_color;
    Color overlay_content_color;

    // @button
    Color button_color;
    Color button_text_color;
    Color button_checked_color;
    Color button_checked_text_color;
    Color button_disabled_color;
    Color button_disabled_text_color;
};

extern Style* g_style;
inline const Style& GetStyle() { return *g_style; }

constexpr Color STYLE_SELECTED_COLOR = Color32ToColor(255, 121, 0, 255);

constexpr float STYLE_BUTTON_PADDING = 8.0f;

constexpr Color STYLE_TEXT_COLOR = Color24ToColor(180,180,170);
constexpr int   STYLE_TEXT_FONT_SIZE = 14;

constexpr Color STYLE_ICON_COLOR = Color24ToColor(180,180,170);

constexpr Color STYLE_ERROR_COLOR = Color24ToColor(0xdf6b6d);



// @mesh
constexpr float STYLE_MESH_EDGE_WIDTH = 0.02f;
constexpr float STYLE_MESH_VERTEX_SIZE = 0.12f;
constexpr float STYLE_MESH_WEIGHT_OUTLINE_SIZE = 0.20f;
constexpr float STYLE_MESH_WEIGHT_SIZE = 0.19f;

// @skeleton
constexpr float STYLE_SKELETON_BONE_WIDTH = 0.02f;
constexpr float STYLE_SKELETON_BONE_RADIUS = 0.06f;
constexpr Color STYLE_SKELETON_BONE_COLOR = COLOR_BLACK;
constexpr float STYLE_SKELETON_PARENT_DASH = 0.1f;



inline Color STYLE_BACKGROUND_COLOR() { return GetStyle().background_color; }
inline Color STYLE_SELECTION_COLOR() { return GetStyle().selection_color; }

inline Color STYLE_BUTTON_COLOR() { return GetStyle().button_color; }
inline Color STYLE_BUTTON_TEXT_COLOR() { return GetStyle().button_text_color; }
inline Color STYLE_BUTTON_CHECKED_COLOR() { return GetStyle().button_checked_color; }
inline Color STYLE_BUTTON_CHECKED_TEXT_COLOR() { return GetStyle().button_checked_text_color; }
inline Color STYLE_BUTTON_DISABLED_COLOR() { return GetStyle().button_disabled_color; }
inline Color STYLE_BUTTON_DISABLED_TEXT_COLOR() { return GetStyle().button_disabled_text_color; }

inline Color STYLE_WORKSPACE_COLOR() { return GetStyle().workspace_color; }
constexpr float STYLE_WORKSPACE_PADDING = 16.0f;

inline Color STYLE_GRID_COLOR() { return GetStyle().grid_color; }

inline Color    STYLE_OVERLAY_BACKGROUND_COLOR() { return GetStyle().overlay_background_color; }
inline Color    STYLE_OVERLAY_TEXT_COLOR() { return GetStyle().overlay_text_color; }
inline int      STYLE_OVERLAY_TEXT_SIZE = 14;
inline Color    STYLE_OVERLAY_ACCENT_TEXT_COLOR() { return GetStyle().overlay_accent_text_color; }
inline Color    STYLE_OVERLAY_ICON_COLOR() { return GetStyle().overlay_icon_color; }
inline Color    STYLE_OVERLAY_CONTENT_COLOR() { return GetStyle().overlay_content_color; }
inline float    STYLE_OVERLAY_CONTENT_BORDER_RADIUS = 9.0f;
constexpr float STYLE_OVERLAY_PADDING = 12.0f;
constexpr float STYLE_OVERLAY_BORDER_RADIUS = 16.0f;


constexpr float STYLE_TOGGLE_BUTTON_HEIGHT = 32.0f;
constexpr float STYLE_TOGGLE_BUTTON_PADDING = 6.0f;
constexpr float STYLE_TOGGLE_BUTTON_BORDER_RADIUS = 8.0f;


constexpr Color STYLE_COLOR_BLACK_2PCT = {0,0,0,0.02f};
constexpr Color STYLE_COLOR_BLACK_5PCT = {0,0,0,0.05f};
constexpr Color STYLE_COLOR_BLACK_10PCT = {0,0,0,0.1f};

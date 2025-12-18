//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once


constexpr Color STYLE_BACKGROUND_COLOR = Color24ToColor(0x262525);
constexpr Color STYLE_BACKGROUND_COLOR_LIGHT = Color24ToColor(0x2E2D2C);

constexpr Color STYLE_SELECTED_COLOR = Color32ToColor(255, 121, 0, 255);

constexpr Color STYLE_BUTTON_BACKGROUND = STYLE_BACKGROUND_COLOR;
constexpr float STYLE_BUTTON_PADDING = 8.0f;

constexpr Color STYLE_TEXT_COLOR = Color24ToColor(180,180,170);
constexpr int   STYLE_TEXT_FONT_SIZE = 14;

constexpr Color STYLE_ICON_COLOR = Color24ToColor(180,180,170);

constexpr Color STYLE_ERROR_COLOR = Color24ToColor(0xdf6b6d);

constexpr Color STYLE_WORKSPACE_COLOR = Color24ToColor(0x3d3c3c);
constexpr float STYLE_WORKSPACE_PADDING = 16.0f;



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

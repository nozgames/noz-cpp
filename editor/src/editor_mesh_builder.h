//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

extern void AddEditorLine(MeshBuilder* builder, const Vec2& v0, const Vec2& v1, float width, const Color& color);
extern void AddEditorSquare(MeshBuilder* builder, const Vec2& center, float size, const Color& color);
extern void AddEditorCircle(MeshBuilder* builder, const Vec2& center, float radius, const Color& color);
extern void AddEditorCircleStroke(MeshBuilder* builder, const Vec2& center, float radius, float thickness, const Color& color);
extern void AddEditorArc(MeshBuilder* builder, const Vec2& center, float radius, float fill_percent, const Color& color);
extern void AddEditorBone(MeshBuilder* builder, const Vec2& a, const Vec2& b, float line_width, const Color& color);
extern void AddEditorDashedLine(MeshBuilder* builder, const Vec2& v0, const Vec2& v1, float width, float dash_length, const Color& color);

//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//

#pragma once

struct Rasterizer {};

extern Rasterizer* CreateRasterizer(Allocator* allocator);

extern void SetTarget(Rasterizer* rasterizer, u8* pixels, int width, int height);
extern void SetClipRect(Rasterizer* rasterizer, int x, int y, int w, int h);
extern void SetColor(Rasterizer* rasterizer, const Color& color);
inline void SetColor(Rasterizer* rasterizer, float r, float g, float b, float a) {
    SetColor(rasterizer, Color{r, g, b, a});
}

extern void BeginPath(Rasterizer* rasterizer);
extern void MoveTo(Rasterizer* rasterizer, float x, float y);
extern void LineTo(Rasterizer* rasterizer, float x, float y);
extern void CurveTo(Rasterizer* rasterizer, float x1, float y1, float x2, float y2, float x3, float y3);
extern void Fill(Rasterizer* rasterizer);

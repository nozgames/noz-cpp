//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
#pragma once

// Platform rasterizer - abstracts the underlying 2D rasterization library
// Currently implemented with AGG (Anti-Grain Geometry) with optional anti-aliasing

struct PlatformRasterizer;

PlatformRasterizer* PlatformCreateRasterizer();
void PlatformDestroyRasterizer(PlatformRasterizer* rasterizer);

void PlatformSetTarget(PlatformRasterizer* rasterizer, u8* pixels, int width, int height);
void PlatformSetClipRect(PlatformRasterizer* rasterizer, int x, int y, int w, int h);
void PlatformSetColor(PlatformRasterizer* rasterizer, float r, float g, float b, float a);
void PlatformSetAntialias(PlatformRasterizer* rasterizer, bool enabled);

void PlatformBeginPath(PlatformRasterizer* rasterizer);
void PlatformMoveTo(PlatformRasterizer* rasterizer, float x, float y);
void PlatformLineTo(PlatformRasterizer* rasterizer, float x, float y);
void PlatformClosePath(PlatformRasterizer* rasterizer);
void PlatformFill(PlatformRasterizer* rasterizer);

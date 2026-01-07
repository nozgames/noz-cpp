//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
#pragma once

// Simple scanline polygon rasterizer with no anti-aliasing
// Outputs premultiplied ARGB directly to a pixel buffer
struct ScanlineRasterizer {};

ScanlineRasterizer* CreateScanlineRasterizer();
void DestroyScanlineRasterizer(ScanlineRasterizer* rasterizer);

void SetTarget(ScanlineRasterizer* rasterizer, u8* pixels, int width, int height);
void SetClipRect(ScanlineRasterizer* rasterizer, int x, int y, int w, int h);
void SetColor(ScanlineRasterizer* rasterizer, float r, float g, float b, float a);

void BeginPath(ScanlineRasterizer* rasterizer);
void MoveTo(ScanlineRasterizer* rasterizer, float x, float y);
void LineTo(ScanlineRasterizer* rasterizer, float x, float y);
void ClosePath(ScanlineRasterizer* rasterizer);
void Fill(ScanlineRasterizer* rasterizer);

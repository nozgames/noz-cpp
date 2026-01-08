//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
//  Scanline rasterizer wrapper - delegates to platform implementation
//

#include "scanline_rasterizer.h"
#include "platform/editor_platform.h"

// ScanlineRasterizer is just a wrapper around PlatformRasterizer
// This allows us to swap the underlying implementation if needed

ScanlineRasterizer* CreateScanlineRasterizer() {
    return (ScanlineRasterizer*)PlatformCreateRasterizer();
}

void DestroyScanlineRasterizer(ScanlineRasterizer* rasterizer) {
    PlatformDestroyRasterizer((PlatformRasterizer*)rasterizer);
}

void SetTarget(ScanlineRasterizer* rasterizer, u8* pixels, int width, int height) {
    PlatformSetTarget((PlatformRasterizer*)rasterizer, pixels, width, height);
}

void SetClipRect(ScanlineRasterizer* rasterizer, int x, int y, int w, int h) {
    PlatformSetClipRect((PlatformRasterizer*)rasterizer, x, y, w, h);
}

void SetColor(ScanlineRasterizer* rasterizer, float r, float g, float b, float a) {
    PlatformSetColor((PlatformRasterizer*)rasterizer, r, g, b, a);
}

void SetAntialias(ScanlineRasterizer* rasterizer, bool enabled) {
    PlatformSetAntialias((PlatformRasterizer*)rasterizer, enabled);
}

void BeginPath(ScanlineRasterizer* rasterizer) {
    PlatformBeginPath((PlatformRasterizer*)rasterizer);
}

void MoveTo(ScanlineRasterizer* rasterizer, float x, float y) {
    PlatformMoveTo((PlatformRasterizer*)rasterizer, x, y);
}

void LineTo(ScanlineRasterizer* rasterizer, float x, float y) {
    PlatformLineTo((PlatformRasterizer*)rasterizer, x, y);
}

void ClosePath(ScanlineRasterizer* rasterizer) {
    PlatformClosePath((PlatformRasterizer*)rasterizer);
}

void Fill(ScanlineRasterizer* rasterizer) {
    PlatformFill((PlatformRasterizer*)rasterizer);
}

//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
//  AGG-based polygon rasterizer implementation (non-AA)
//

#include "editor_platform.h"

// Disable warnings for AGG library headers
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244) // conversion, possible loss of data
#pragma warning(disable: 5054) // operator '|': deprecated between enumerations of different types
#pragma warning(disable: 5055) // operator: deprecated between enumerations and floating-point types
#endif

#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_renderer_base.h"
#include "agg_renderer_scanline.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_scanline_p.h"
#include "agg_path_storage.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

struct PlatformRasterizer {
    agg::rendering_buffer rbuf;
    agg::rasterizer_scanline_aa<> ras;
    agg::scanline_p8 sl;
    agg::path_storage path;

    u8* pixels = nullptr;
    int width = 0;
    int height = 0;
    int clip_x = 0;
    int clip_y = 0;
    int clip_w = 0;
    int clip_h = 0;

    // Fill color (premultiplied RGBA8)
    agg::rgba8 fill_color;
};

PlatformRasterizer* PlatformCreateRasterizer() {
    return new PlatformRasterizer();
}

void PlatformDestroyRasterizer(PlatformRasterizer* rasterizer) {
    delete rasterizer;
}

void PlatformSetTarget(PlatformRasterizer* rasterizer, u8* pixels, int width, int height) {
    rasterizer->pixels = pixels;
    rasterizer->width = width;
    rasterizer->height = height;
    rasterizer->clip_x = 0;
    rasterizer->clip_y = 0;
    rasterizer->clip_w = width;
    rasterizer->clip_h = height;

    // AGG uses negative stride for top-down buffers, positive for bottom-up
    // Our buffer is top-down, so use positive stride (width * 4 bytes per pixel)
    rasterizer->rbuf.attach(pixels, width, height, width * 4);
}

void PlatformSetClipRect(PlatformRasterizer* rasterizer, int x, int y, int w, int h) {
    rasterizer->clip_x = x;
    rasterizer->clip_y = y;
    rasterizer->clip_w = w;
    rasterizer->clip_h = h;

    // Set clip box on rasterizer
    rasterizer->ras.clip_box(x, y, x + w, y + h);
}

void PlatformSetColor(PlatformRasterizer* rasterizer, float r, float g, float b, float a) {
    // Convert to premultiplied RGBA8
    u8 alpha = (u8)(a * 255.0f);
    u8 red = (u8)(r * a * 255.0f);
    u8 green = (u8)(g * a * 255.0f);
    u8 blue = (u8)(b * a * 255.0f);
    rasterizer->fill_color = agg::rgba8(red, green, blue, alpha);
}

void PlatformBeginPath(PlatformRasterizer* rasterizer) {
    rasterizer->path.remove_all();
    rasterizer->ras.reset();
}

void PlatformMoveTo(PlatformRasterizer* rasterizer, float x, float y) {
    rasterizer->path.move_to(x, y);
}

void PlatformLineTo(PlatformRasterizer* rasterizer, float x, float y) {
    rasterizer->path.line_to(x, y);
}

void PlatformClosePath(PlatformRasterizer* rasterizer) {
    rasterizer->path.close_polygon();
}

void PlatformFill(PlatformRasterizer* rasterizer) {
    // Add path to rasterizer
    rasterizer->ras.add_path(rasterizer->path);

    // Set up pixel format and renderer for premultiplied BGRA (native-endian ARGB on little-endian)
    // order_bgra stores as B,G,R,A bytes which is ARGB as a 32-bit value on little-endian
    typedef agg::pixfmt_alpha_blend_rgba<agg::blender_rgba_pre<agg::rgba8, agg::order_bgra>, agg::rendering_buffer> pixfmt_type;
    pixfmt_type pixf(rasterizer->rbuf);
    agg::renderer_base<pixfmt_type> ren_base(pixf);

    // Use bin_solid for non-antialiased rendering
    agg::render_scanlines_bin_solid(rasterizer->ras, rasterizer->sl, ren_base, rasterizer->fill_color);
}

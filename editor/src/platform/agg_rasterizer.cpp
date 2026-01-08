//
//  NoZ - Copyright(c) 2026 NoZ Games, LLC
//
//  AGG-based polygon rasterizer with optional anti-aliasing
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

// Custom blender that avoids dark halos when rendering onto transparent backgrounds.
// When destination alpha is 0, copies source color directly instead of blending with black.
// This preserves correct edge colors for anti-aliased rendering.
template<class ColorT, class Order>
struct blender_rgba_pre_no_halo : agg::conv_rgba_pre<ColorT, Order> {
    typedef ColorT color_type;
    typedef Order order_type;
    typedef typename color_type::value_type value_type;
    typedef typename color_type::calc_type calc_type;
    enum { base_shift = color_type::base_shift };
    enum { base_mask = color_type::base_mask };

    // Partial coverage blend (with cover parameter) - called by pixfmt for AA edges
    static AGG_INLINE void blend_pix(value_type* p,
                          value_type cr, value_type cg, value_type cb, value_type alpha,
                          agg::cover_type cover) {
        blend_pix(p,
            color_type::mult_cover(cr, cover),
            color_type::mult_cover(cg, cover),
            color_type::mult_cover(cb, cover),
            color_type::mult_cover(alpha, cover));
    }

    // Full coverage blend (no cover parameter)
    static AGG_INLINE void blend_pix(value_type* p,
                          value_type cr, value_type cg, value_type cb, value_type alpha) {
        calc_type da = p[Order::A];
        if (da == 0) {
            // Destination is transparent - copy source directly (avoids dark halo)
            p[Order::R] = cr;
            p[Order::G] = cg;
            p[Order::B] = cb;
            p[Order::A] = alpha;
        } else {
            // Normal premultiplied alpha blend using prelerp: dst = src + dst * (1 - src_alpha)
            p[Order::R] = color_type::prelerp(p[Order::R], cr, alpha);
            p[Order::G] = color_type::prelerp(p[Order::G], cg, alpha);
            p[Order::B] = color_type::prelerp(p[Order::B], cb, alpha);
            p[Order::A] = color_type::prelerp(p[Order::A], alpha, alpha);
        }
    }
};

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

    // Anti-aliasing mode
    bool antialias = false;
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

void PlatformSetAntialias(PlatformRasterizer* rasterizer, bool enabled) {
    rasterizer->antialias = enabled;
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
    if (rasterizer->antialias) {
        // Use custom blender that avoids dark halos on transparent backgrounds
        typedef agg::pixfmt_alpha_blend_rgba<blender_rgba_pre_no_halo<agg::rgba8, agg::order_bgra>, agg::rendering_buffer> pixfmt_type;
        pixfmt_type pixf(rasterizer->rbuf);
        agg::renderer_base<pixfmt_type> ren_base(pixf);
        agg::render_scanlines_aa_solid(rasterizer->ras, rasterizer->sl, ren_base, rasterizer->fill_color);
    } else {
        // Standard premultiplied blender for non-AA rendering
        typedef agg::pixfmt_alpha_blend_rgba<agg::blender_rgba_pre<agg::rgba8, agg::order_bgra>, agg::rendering_buffer> pixfmt_type;
        pixfmt_type pixf(rasterizer->rbuf);
        agg::renderer_base<pixfmt_type> ren_base(pixf);
        agg::render_scanlines_bin_solid(rasterizer->ras, rasterizer->sl, ren_base, rasterizer->fill_color);
    }
}

//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "screen.h"

constexpr int OUTPUT_BUFFER_SIZE = 1024 * 1024 * 16;

struct Pixel
{
    char value;
    tcolor_t color;
    tcolor_t bg_color;
};

struct Clip
{
    irect_t rect;
    bool wrap;
};

static Pixel* g_buffer = nullptr;
static char g_output_buffer[OUTPUT_BUFFER_SIZE];
static i32 g_screen_width = 0;
static i32 g_screen_height = 0;
static ivec2 g_cursor = {0,0};
static std::vector<Clip> g_clip;
static color24_t g_colors[TCOLOR_MAX] = {};

static irect_t Clip(const irect_t& rect)
{
    assert(g_clip.size() > 0);
    auto& clip = g_clip.back().rect;
    irect_t r;
    r.x = max(rect.x, clip.x);
    r.y = max(rect.y, clip.y);
    r.width = min(rect.x + rect.width, clip.x + clip.width) - r.x;
    r.height = min(rect.y + rect.height, clip.y + clip.height) - r.y;
    return r;
}

static ivec2 Clip(const ivec2& pt)
{
    assert(g_clip.size() > 0);
    auto& clip = g_clip.back().rect;
    ivec2 r;
    r.x = clamp(pt.x, clip.x, GetRight(clip) - 1);
    r.y = clamp(pt.y, clip.y, GetBottom(clip) - 1);
    return r;
}

int GetScreenWidth()
{
    return g_screen_width;
}

int GetScreenHeight()
{
    return g_screen_height;
}

void MoveCursor(int x, int y)
{
    g_cursor = Clip(ivec2(x,y));
}

void SetPixel(i32 x, i32 y, char value, tcolor_t color, tcolor_t bg_color)
{
    auto& clip = g_clip.back();
    auto clip_l = clip.rect.x;
    auto clip_t = clip.rect.y;
    auto clip_r = clip.rect.x + clip.rect.width;
    auto clip_b = clip.rect.y + clip.rect.height;

    if (x < clip_l || x >= clip_r || y < clip_t || y >= clip_b)
        return;

    g_buffer[y * g_screen_width + x] = { value, color, bg_color };

    MoveCursor(x+1, y);
    if (clip.wrap && g_cursor.x >= clip_r)
        MoveCursor(clip_l, g_cursor.y + 1);
}

void SetPixels(i32 x, i32 y, const char* str, tcolor_t color)
{
    assert(str);
    if (*str == 0)
        return;

    SetPixel(x++, y, *str, color);
    for (++str;*str; ++str)
        AddPixel(*str, color);
}

void AddPixel(char c, tcolor_t color)
{
    SetPixel(g_cursor.x, g_cursor.y, c, color);
}

void AddPixels(const char* str, tcolor_t color)
{
    assert(str);
    for (;*str; str++)
        AddPixel(*str, color);
}

void PushClipRect(const irect_t& rect, bool wrap)
{
    g_clip.push_back({rect, wrap});
}

void PopClipRect()
{
    assert(g_clip.size() > 0);
    g_clip.pop_back();
}

void DrawVerticalLine(i32 x, i32 y, i32 height, char c, tcolor_t color, tcolor_t bg_color)
{
    auto& clip = g_clip.back();
    auto clip_l = clip.rect.x;
    auto clip_t = clip.rect.y;
    auto clip_r = clip.rect.x + clip.rect.width;
    auto clip_b = clip.rect.y + clip.rect.height;

    x = clamp(x, clip_l, clip_r);
    y = clamp(y, clip_t, clip_b);
    auto yy = min(x + height, clip_b);
    for (;y<yy; y++)
        SetPixel(x, y, c, color, bg_color);
}

void DrawHorizontalLine(i32 x, i32 y, i32 width, char c, tcolor_t color, tcolor_t bg_color)
{
    auto& clip = g_clip.back();
    auto clip_l = clip.rect.x;
    auto clip_t = clip.rect.y;
    auto clip_r = clip.rect.x + clip.rect.width;
    auto clip_b = clip.rect.y + clip.rect.height;

    x = clamp(x, clip_l, clip_r);
    y = clamp(y, clip_t, clip_b);
    for (auto xx = min(x + width, clip_r); x<xx; x++)
        SetPixel(x, y, c, color, bg_color);
}

void SetBackgroundColor(const irect_t& rect, tcolor_t color)
{
    auto clip = Clip(rect);
    for (int y = clip.y, yy = GetBottom(clip); y < yy; y++)
        for (int x = clip.x, xx = GetRight(clip); x < xx; x++)
            g_buffer[y * g_screen_width + x].bg_color = color;
}

void UpdateScreenSize(i32 width, i32 height)
{
    g_clip.clear();
    g_clip.push_back({0,0,width,height});

    auto old_buffer = g_buffer;
    auto new_buffer = (Pixel*)calloc(1, width * height * sizeof(Pixel));

    if (old_buffer)
    {
        i32 yy = min(height, g_screen_height);
        i32 xx = min(width, g_screen_width);
        for (int y = 0; y < yy; y++)
        {
            auto old_row = old_buffer + y * g_screen_width;
            auto new_row = new_buffer + y * width;
            memcpy(new_row, old_row, xx * sizeof(Pixel));
        }

        free(old_buffer);
    }

    g_buffer = new_buffer;
    g_screen_width = width;
    g_screen_height = height;
}

void ClearScreen(char c, tcolor_t color)
{
    assert(g_clip.size() > 0);
    assert(g_buffer);

    auto& clip = g_clip.back();
    auto clip_l = clip.rect.x;
    auto clip_t = clip.rect.y;
    auto clip_r = GetRight(clip.rect);
    auto clip_b = GetBottom(clip.rect);

    if (clip_l >= clip_r || clip_t >= clip_b)
        return;

    for (int y = clip_t; y < clip_b; y++)
        for (int x = clip_l; x < clip_r; x++)
            g_buffer[y * g_screen_width + x] = { c, color };

    MoveCursor(clip_l, clip_t);
}

static char* RenderEscape(char* o)
{
    *(o++) = '\033';
    *(o++) = '[';
    return o;
}

static char* RenderInt(char* o, int i)
{
    auto len = snprintf(o, OUTPUT_BUFFER_SIZE - (o - g_output_buffer)-1, "%d", i);
    o += len;;
    return o;
}

static char* RenderColor(char* o, tcolor_t color)
{
    if (color == TCOLOR_NONE)
    {
        o = RenderEscape(o);
        *o++ = '3';
        *o++ = '9';
        *o++ = 'm';
        return o;
    }

    o = RenderEscape(o);
    *o++ = '3';
    *o++ = '8';
    *o++ = ';';
    *o++ = '2';
    *o++ = ';';
    o = RenderInt(o, g_colors[color].r);
    *o++ = ';';
    o = RenderInt(o, g_colors[color].g);
    *o++ = ';';
    o = RenderInt(o, g_colors[color].b);
    *o++ = 'm';
    return o;
}

static char* RenderBackgroundColor(char* o, tcolor_t color)
{
    if (color == TCOLOR_NONE)
    {
        o = RenderEscape(o);
        *o++ = '4';
        *o++ = '9';
        *o++ = 'm';
        return o;
    }

    o = RenderEscape(o);
    *o++ = '4';
    *o++ = '8';
    *o++ = ';';
    *o++ = '2';
    *o++ = ';';
    o = RenderInt(o, g_colors[color].r);
    *o++ = ';';
    o = RenderInt(o, g_colors[color].g);
    *o++ = ';';
    o = RenderInt(o, g_colors[color].b);
    *o++ = 'm';
    return o;
}

static char* RenderMoveCursor(char* o, int x, int y)
{
    o = RenderEscape(o);
    o = RenderInt(o, y + 1);
    *(o++) = ';';
    o = RenderInt(o, x + 1);
    *(o++) = 'H';
    return o;
}

ScreenOutputBuffer RenderScreen()
{
    auto o = g_output_buffer;
    tcolor_t fg_color = TCOLOR_NONE;
    tcolor_t bg_color = TCOLOR_NONE;
    o = RenderColor(o, TCOLOR_NONE);
    o = RenderBackgroundColor(o, TCOLOR_NONE);
    for (auto y = 0; y < g_screen_height; y++)
    {
        Pixel* p = g_buffer + y * g_screen_width;
        o = RenderMoveCursor(o, 0, y);
        for (auto x = 0; x < g_screen_width; x++, p++)
        {
            if (p->color != fg_color)
            {
                fg_color = p->color;
                o = RenderColor(o, p->color);
            }

            if (p->bg_color != bg_color)
            {
                bg_color = p->bg_color;
                o = RenderBackgroundColor(o, p->bg_color);
            }

            *o++ = p->value;
        }
    }

    return { g_output_buffer, (size_t)((u8*)o - (u8*)g_output_buffer) };
}

void InitScreen(i32 width, i32 height)
{
    // Initialize color table
    g_colors[TCOLOR_BLACK] = { 0, 0, 0 };
    g_colors[TCOLOR_RED] = { 255, 0, 0 };
    g_colors[TCOLOR_GREEN] = { 0, 255, 0 };
    g_colors[TCOLOR_YELLOW] = { 255, 255, 0 };
    g_colors[TCOLOR_BLUE] = { 0, 0, 255 };
    g_colors[TCOLOR_MAGENTA] = { 255, 0, 255 };
    g_colors[TCOLOR_CYAN] = { 0, 255, 255 };
    g_colors[TCOLOR_WHITE] = { 255, 255, 255 };
    g_colors[TCOLOR_GRAY] = { 128, 128, 128 };
    g_colors[TCOLOR_LIGHT_GRAY] = { 192, 192, 192 };

    UpdateScreenSize(width, height);
    ClearScreen();
}

void ShutdownScreen()
{
    if (g_buffer)
        free(g_buffer);

    g_buffer = nullptr;
    g_screen_width = 0;
    g_screen_height = 0;
}

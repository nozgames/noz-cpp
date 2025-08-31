//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

typedef u8 tcolor_t;

struct ScreenOutputBuffer
{
    char* buffer;
    size_t size;
};

constexpr tcolor_t TCOLOR_NONE = 0;
constexpr tcolor_t TCOLOR_BLACK = 1;
constexpr tcolor_t TCOLOR_WHITE = 2;
constexpr tcolor_t TCOLOR_RED = 3;
constexpr tcolor_t TCOLOR_GREEN = 4;
constexpr tcolor_t TCOLOR_BLUE = 5;
constexpr tcolor_t TCOLOR_YELLOW = 6;
constexpr tcolor_t TCOLOR_MAGENTA = 7;
constexpr tcolor_t TCOLOR_CYAN = 8;
constexpr tcolor_t TCOLOR_GRAY = 9;
constexpr tcolor_t TCOLOR_LIGHT_GRAY = 10;

constexpr tcolor_t TCOLOR_MAX = 255;

constexpr tcolor_t TCOLOR_DISABLED = TCOLOR_GRAY;

void ClearScreen(char c=' ', tcolor_t color=TCOLOR_NONE);
i32 GetScreenWidth();
i32 GetScreenHeight();

void SetPixel(i32 x, i32 y, char c, tcolor_t color = TCOLOR_NONE, tcolor_t bg_color = TCOLOR_NONE);
void SetPixels(i32 x, i32 y, const char* str, tcolor_t color = TCOLOR_NONE);
void AddPixels(const char* str, tcolor_t color = TCOLOR_NONE);
void AddPixel(char c, tcolor_t color = TCOLOR_NONE);
void DrawVerticalLine(i32 x, i32 y, i32 height, char c, tcolor_t color, tcolor_t bg_color = TCOLOR_NONE);
void DrawHorizontalLine(i32 x, i32 y, i32 width, char c, tcolor_t color, tcolor_t bg_color = TCOLOR_NONE);
void SetBackgroundColor(const irect_t& rect, tcolor_t color);
void MoveCursor(int x, int y);

void PushClipRect(const irect_t& rect, bool wrap=false);
void PopClipRect();

color24_t GetTColor(tcolor_t color);
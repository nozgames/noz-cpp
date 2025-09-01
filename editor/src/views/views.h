//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

// @structs
struct View
{
    const struct ViewTraits* traits;
};

// @traits
struct ViewTraits
{
    void(*render)(View* view, const RectInt& rect);
};

// @log_view
struct LogView : public View
{
    RingBuffer* messages = nullptr;
};

LogView* CreateLogView(Allocator* allocator);

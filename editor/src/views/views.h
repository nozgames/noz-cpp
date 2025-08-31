//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct View : Object {};
struct LogView : View {};

// @view_traits

struct ViewTraits
{
    void(*render)(View* view, const irect_t& rect);
};

extern ViewTraits g_view_traits[];

// @view
View* CreateView(Allocator* allocator, size_t view_size, type_t view_type);

// @log_view
LogView* CreateLogView(Allocator* allocator);

// void Clear(LogView* view);
// void Render(LogView* view, const irect_t& rect);
// void Add(const std::string& message);
// size_t Count() const;
// void SetMax(size_t max_messages);

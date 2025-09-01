//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "views.h"

constexpr int MAX_LOG_MESSAGES = 1024;
constexpr int MAX_LOG_MESSAGE_LENGTH = 1024;

struct LogMessage
{
    tchar_t value[MAX_LOG_MESSAGE_LENGTH];
    u32 length;
};

void AddMessage(LogView* view, const char* str)
{
    if (IsFull(view->messages))
        PopFront(view->messages);

    auto item = (LogMessage*)PushBack(view->messages);
    CStringToTChar(str, item->value, MAX_LOG_MESSAGE_LENGTH);
}

void LogViewRender(View* view, const RectInt& rect)
{
    LogView* log_view = (LogView*)view;
    u32 count = GetCount(log_view->messages);
    u32 bot = GetBottom(rect) - 1;
    u32 top = GetTop(rect);
    // for (u32 i=0; i<count && bot - i >= top; i++)
    //     AddPixels()

    // TODO: need tstring working
}

static ViewTraits g_log_view_traits = {
    .render = LogViewRender
};

LogView* CreateLogView(Allocator* allocator)
{
    LogView* view = (LogView*)Alloc(allocator, sizeof(LogView));
    view->traits = &g_log_view_traits;
    view->messages = CreateRingBuffer(allocator, MAX_LOG_MESSAGES, sizeof(LogMessage));
    return view;
}

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

struct LogViewImpl
{
    RingBuffer* messages = nullptr;
};

static LogViewImpl* Impl(LogView* view) { return (LogViewImpl*)Cast(view, EDITOR_TYPE_LOG_VIEW); }

#if 0
LogView::LogView()
{
    _tree_view = std::make_unique<TreeView>();
}

void LogView::Clear()
{
    _tree_view->Clear();
}

void LogView::Add(const std::string& message)
{
    auto builder = CreateTStringBuilder(ALLOCATOR_SCRATCH);

    //_tree_view->Add(TStringBuilder().Add(message).ToString(), 0, nullptr);
    Destroy(builder);
}

size_t LogView::Count() const
{
    return _tree_view->NodeCount();
}

void LogView::SetMax(size_t max_messages)
{
    _tree_view->SetMaxEntries(max_messages);
}

void LogView::Render(const irect_t& rect)
{
    _tree_view->Render(rect);
}

#endif

void AddMessage(LogView* view, const char* str)
{
    auto impl = Impl(view);

    if (IsFull(impl->messages))
        PopFront(impl->messages);

    auto item = (LogMessage*)PushBack(impl->messages);
    CStringToTChar(str, item->value, MAX_LOG_MESSAGE_LENGTH);
}

void LogViewRender(View* view, const irect_t& rect)
{
}

LogView* CreateLogView(Allocator* allocator)
{
    g_view_traits[EDITOR_TYPE_LOG_VIEW] = {
        .render = LogViewRender
    };

    LogView* view = (LogView*)CreateView(allocator, sizeof(LogViewImpl), EDITOR_TYPE_LOG_VIEW);
    LogViewImpl* impl = Impl(view);
    impl->messages = CreateRingBuffer(allocator, MAX_LOG_MESSAGES, sizeof(LogMessage));
    return view;
}

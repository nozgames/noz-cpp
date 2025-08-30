//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "tui/terminal.h"
#include "tui/text_input.h"
#include "tui/screen.h"
#include "views/log_view.h"
#include "views/view_interface.h"
#include "views/inspector_view.h"
#include "views/inspector_object.h"

bool InitImporter();
void ShutdownImporter();
bool IsImporterRunning();

struct Editor
{
    std::unique_ptr<LogView> log_view;
    std::stack<std::unique_ptr<IView>> view_stack;
    TextInput* command_input;
    TextInput* search_input;
    bool command_mode;
    bool search_mode;
    std::atomic<bool> is_running;
};

static Editor g_editor = {};

static IView* GetView()
{
    if (g_editor.view_stack.empty())
        return g_editor.log_view.get();

    return g_editor.view_stack.top().get();
}

static void PushView(std::unique_ptr<IView> view)
{
    assert(view && view.get());
    g_editor.view_stack.push(std::move(view));
}

static void PopView()
{
    assert(g_editor.view_stack.size() > 0);
    auto current = g_editor.view_stack.top().get();
    if (!current->CanPopFromStack())
        return;

    g_editor.view_stack.pop();
}

void HandleInspectorObject(std::unique_ptr<InspectorObject> object)
{
    auto current_view = GetView();
    auto inspector = dynamic_cast<InspectorView*>(current_view);

    if (inspector && object)
    {
        try
        {
            // Set the root object in the inspector
            inspector->SetRootObject(std::move(object));
        }
        catch (...)
        {
            LogWarning("Exception occurred while populating inspector");
            inspector->ClearTree();
            //inspector->AddLine("Error processing inspection data");
        }
    }
    else if (inspector)
    {
        LogInfo("Inspector exists but no data - showing no-data message");
        // Inspector exists but no data received - update waiting message
        inspector->ClearTree();
        //inspector->AddLine("No client connected or no data received");
    }
}

// Handle case where no inspection response is received
void HandleNoInspectionResponse()
{
    IView* current_view = GetView();
    InspectorView* inspector = dynamic_cast<InspectorView*>(current_view);
    
    if (inspector)
    {
        inspector->ClearTree();
        //inspector->AddLine("No client connected");
    }
}

struct DebugLog
{
    std::mutex mutex;
    FILE* file = nullptr;
};

static DebugLog& GetDebugLog()
{
    static DebugLog instance;
    return instance;
}

// Thread safety for log messages
static std::thread::id g_main_thread_id;

struct LogQueue
{
    std::mutex mutex;
    std::queue<std::string> queue;
};

static LogQueue& GetLogQueue()
{
    static LogQueue instance;
    return instance;
}

static void HandleResize(int new_width, int new_height)
{
    //g_editor.command_input = std::make_unique<text_input>(1, new_height - 1, new_width - 1);
}

static void HandleLog(LogType type, const char* message)
{
    // Handle debug logging to file (thread-safe)
    if (type == LOG_TYPE_DEBUG)
    {
        DebugLog& debug_log = GetDebugLog();
        std::lock_guard<std::mutex> lock(debug_log.mutex);
        
        if (!debug_log.file)
            debug_log.file = fopen("debug.log", "w");

        if (debug_log.file)
        {
            fprintf(debug_log.file, "[DEBUG] %s\n", message);
            fflush(debug_log.file);
        }
        return;
    }

    // Add type prefix with color for display
    std::string formatted_message;
    switch(type) {
    case LOG_TYPE_INFO: 
        formatted_message = std::string(message); 
        break;
    case LOG_TYPE_WARNING: 
        // Nice yellow (not fully saturated) - RGB(200, 180, 0)
        formatted_message = "\033[38;2;200;180;0m[WARNING]\033[0m " + std::string(message); 
        break;
    case LOG_TYPE_ERROR: 
        // Nice red (not fully saturated) - RGB(200, 80, 80)
        formatted_message = "\033[38;2;200;80;80m[ERROR]\033[0m " + std::string(message); 
        break;
    default: 
        formatted_message = std::string(message); 
        break;
    }

    // Check if we're on the main thread
    if (std::this_thread::get_id() == g_main_thread_id)
    {
        // On main thread - add directly to log view
        g_editor.log_view->Add(formatted_message);
        RequestRender();
    }
    else
    {
        // On background thread - queue the message safely
        LogQueue& log_queue = GetLogQueue();
        std::lock_guard<std::mutex> lock(log_queue.mutex);
        log_queue.queue.push(formatted_message);
        // Don't call RequestRender() from background thread
    }
}

static void ProcessQueuedLogMessages()
{
    LogQueue& log_queue = GetLogQueue();
    std::lock_guard<std::mutex> lock(log_queue.mutex);
    bool render = false;
    while (!log_queue.queue.empty())
    {
        std::string message = log_queue.queue.front();
        log_queue.queue.pop();
        g_editor.log_view->Add(message);
        render = true;
    }

    if (render)
        RequestRender();
}

static void DrawStatusBar(const irect_t& rect)
{
    static auto title = "NoZ Editor";
    static auto cmd_mode = " - Command Mode";

    auto line = GetBottom(rect) - 2;
    SetPixels(rect.x, GetBottom(rect) - 2, title, TCOLOR_BLACK);

    if (g_editor.command_mode)
        AddPixels(cmd_mode, TCOLOR_BLACK);

    SetBackgroundColor({rect.x, line, rect.width, 1}, TCOLOR_LIGHT_GRAY);
}

static void DrawCommandLine(const irect_t& rect)
{
    if (g_editor.search_mode)
    {
        SetPixel(rect.x, rect.y, '/');
        Render(g_editor.search_input);
    }
    else if (g_editor.command_mode)
    {
        SetPixel(rect.x, rect.y, ':');
        Render(g_editor.command_input);
    }
#if 0
    else
    {
        // Check if there's an active search to display
        std::string search_text = GetText(g_editor.search_input);
        if (!search_text.empty())
        {
            AddChar('/');
            AddString(search_text.c_str());
            AddString(" (filtered - ESC to clear)");
        }
        
        // Fill remaining space
        int used_chars = GetCursorX();
        for (int i = used_chars; i < width; i++)
            AddChar(' ');
    }
#endif
}


static void HandleCommand(const std::string& command)
{
    if (command == "q" || command == "quit")
    {
        // If there are views on the stack, pop one instead of quitting
        if (!g_editor.view_stack.empty())
        {
            PopView();
        }
        else
        {
            g_editor.is_running = false;
        }
    }
    else if (command == "clear")
    {
        g_editor.log_view->Clear();
    }
    else if (command == "i" || command == "inspector")
    {
        // Check if current view is already an inspector
        auto* current_view = GetView();
        auto* existing_inspector = dynamic_cast<InspectorView*>(current_view);
        
        if (existing_inspector)
        {
            existing_inspector->ClearTree();
            existing_inspector->ResetRequestState(); // Reset so it will send a new request
        }
        else
        {
            // Push a new inspector view onto the stack
            PushView(std::unique_ptr<IView>(new InspectorView()));
        }
    }
    else if (!command.empty())
    {
        LogInfo("Unknown command: %s", command.c_str());
        LogInfo("Available commands: clear, quit, tree (t), inspector (i)");
    }
}

static void RunEditor()
{
    // Store main thread ID for thread safety
    g_main_thread_id = std::this_thread::get_id();
    
    InitLog(HandleLog);
    LogInfo("NoZ Editor starting...");
    LogInfo("Auto-starting asset importer...");

    InitImporter();
    RenderTerminal();

    while (g_editor.is_running)
    {
        // Process any queued log messages from background threads
        ProcessQueuedLogMessages();
        
        UpdateTerminal();

        int key = GetTerminalKey();
        if (key == ERR)
        {
            RenderTerminal();
            std::this_thread::yield();
            continue;
        }
        
        // Handle mouse events (including scroll) to prevent terminal scrolling
        if (key == KEY_MOUSE)
        {
            // Just consume the mouse event to prevent terminal scrolling
            // TODO: Implement proper mouse/scroll handling later
            continue; // Don't process mouse events as regular keys
        }

        if (g_editor.search_mode)
        {
            if (key == '\n' || key == '\r')
            {
                // Finish search input but keep the filtered view
                g_editor.search_mode = false;
                SetActive(g_editor.search_input, false);
                
                // Show cursor in current view when exiting search mode
                IView* current_view = GetView();
                current_view->SetCursorVisible(true);
            }
            else if (key == 27)
            { // Escape
                // Cancel search and return to full unfiltered view
                g_editor.search_mode = false;
                SetActive(g_editor.search_input, false);
                Clear(g_editor.search_input);
                
                IView* current_view = GetView();
                current_view->ClearSearch();
                current_view->SetCursorVisible(true);
            }
            else
            {
                HandleKey(g_editor.search_input, key);
                
                // Update search in real-time
                std::string pattern = GetText(g_editor.search_input);
                IView* current_view = GetView();
                current_view->SetSearchPattern(pattern);
            }
        }
        else if (g_editor.command_mode)
        {
            if (key == '\n' || key == '\r')
            {
                std::string command = GetText(g_editor.command_input);
                HandleCommand(command);
                g_editor.command_mode = false;
                SetActive(g_editor.command_input, false);
                Clear(g_editor.command_input);
                
                // Show cursor in current view when exiting command mode
                IView* current_view = GetView();
                current_view->SetCursorVisible(true);
            }
            else if (key == 27)
            { // Escape
                g_editor.command_mode = false;
                SetActive(g_editor.command_input, false);
                Clear(g_editor.command_input);
                
                // Show cursor in current view when exiting command mode
                IView* current_view = GetView();
                current_view->SetCursorVisible(true);
            }
            else
            {
                HandleKey(g_editor.command_input, key);
            }
        }
        else
        {
            IView* current_view = GetView();
            
            if (key == '/')
            {
                // Start search mode if current view supports it
                if (current_view->SupportsSearch())
                {
                    g_editor.search_mode = true;
                    SetActive(g_editor.search_input, true);
                    Clear(g_editor.search_input);
                    SetCursorVisible(true);
                    
                    // Hide cursor in current view when entering search mode
                    current_view->SetCursorVisible(false);
                }
            }
            else if (key == ':')
            {
                g_editor.command_mode = true;
                SetActive(g_editor.command_input, true);
                Clear(g_editor.command_input);
                SetCursorVisible(true);
                
                // Hide cursor in current view when entering command mode
                current_view->SetCursorVisible(false);
            }
            else if (key == 'q')
            {
                g_editor.is_running = false;
            }
            else if (key == 27)  // Escape - clear search or pop view from stack
            {
                // Check if there's an active search to clear
                std::string search_text = GetText(g_editor.search_input);
                if (!search_text.empty())
                {
                    // Clear active search
                    Clear(g_editor.search_input);
                    IView* current_view = GetView();
                    current_view->ClearSearch();
                }
                else
                {
                    // No search to clear, pop view from stack
                    PopView();
                }
            }
            else
            {
                // Route input to current view
                if (!current_view->HandleKey(key))
                {
                    // View didn't handle the key, could add default handling here
                }
            }
        }

        RenderTerminal();
    }
}


void RenderEditor(const irect_t& rect)
{
    ClearScreen();
    DrawStatusBar(rect);
    DrawCommandLine({rect.x, rect.height - 1, rect.width, 1});

    auto view = GetView();
    if (view != nullptr)
        view->Render({rect.x, rect.y, rect.width, rect.height - 2});

#if 0
    // Always render the log view as the base layer
    g_editor.log_view.Render(rect);
    
    // If there are views on the stack, render the top one instead
    if (!g_editor.view_stack.empty())
    {
        IView* current_view = g_editor.view_stack.top();
        current_view->Render({rect.x, rect.y, rect.width, rect.height - 2});
    }
    
    // Hide the terminal cursor when not in command/search mode since views handle their own cursor display
    if (!g_editor.command_mode && !g_editor.search_mode)
    {
        SetCursorVisible(false);
    }
#endif
}

void InitEditor()
{
    g_scratch_allocator = CreateArenaAllocator(32 * noz::MB, "scratch");

    InitLog(HandleLog);
    InitTerminal();
    SetRenderCallback([](int width, int height) { RenderEditor({0, 0, width, height}); });
    SetResizeCallback([](int new_width, int new_height) { HandleResize(new_width, new_height); });
    int term_height = GetScreenHeight();
    int term_width = GetScreenWidth();
    g_editor.log_view = std::unique_ptr<LogView>(new LogView());
    g_editor.command_input = CreateTextInput(1, term_height - 1, term_width - 1);
    g_editor.search_input = CreateTextInput(1, term_height - 1, term_width - 1);
    g_editor.is_running = true;
}

void ShutdownEditor()
{
    Destroy(g_editor.command_input);
    Destroy(g_editor.search_input);
    ShutdownImporter();
    ShutdownTerminal();

    // Close debug log file (thread-safe)
    DebugLog& debug_log = GetDebugLog();
    std::lock_guard<std::mutex> lock(debug_log.mutex);
    if (debug_log.file)
    {
        fclose(debug_log.file);
        debug_log.file = nullptr;
    }
}

int main(int argc, char* argv[])
{
    InitEditor();
    RunEditor();
    ShutdownEditor();
    return 0;
}

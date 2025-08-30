//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "tui/terminal.h"
#include "tui/text_input.h"
#include "views/log_view.h"
#include "views/tree_view.h"
#include "views/view_interface.h"
#include "views/inspector_view.h"
#include "views/inspector_object.h"
#include <stack>

bool InitImporter();
void ShutdownImporter();
bool IsImporterRunning();

struct Editor
{
    LogView log_view;
    std::stack<IView*> view_stack;
    TextInput* command_input;
    TextInput* search_input;
    bool command_mode = false;
    bool search_mode = false;
    std::atomic<bool> is_running = true;
};

static Editor g_editor = {};

// View stack management functions
static IView* GetCurrentView()
{
    if (g_editor.view_stack.empty())
        return &g_editor.log_view;  // Log view is always the base
    return g_editor.view_stack.top();
}

static void PushView(IView* view)
{
    if (view && view != &g_editor.log_view)  // Never push log view as it's always the base
        g_editor.view_stack.push(view);
}

static void PopView()
{
    if (!g_editor.view_stack.empty())
    {
        IView* current = g_editor.view_stack.top();
        if (current->CanPopFromStack())
        {
            g_editor.view_stack.pop();
            // Delete the popped view if it's not the base log view
            if (current != &g_editor.log_view)
            {
                delete current;
            }
        }
    }
}

// Forward inspection data to active inspector view
void HandleInspectorObject(std::unique_ptr<InspectorObject> object)
{
    auto current_view = GetCurrentView();
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
            inspector->AddLine("Error processing inspection data");
        }
    }
    else if (inspector)
    {
        LogInfo("Inspector exists but no data - showing no-data message");
        // Inspector exists but no data received - update waiting message
        inspector->ClearTree();
        inspector->AddLine("No client connected or no data received");
        LogInfo("Updated inspector with no-data message");
    }
    else
    {
        LogInfo("No active inspector view to receive data");
    }
}

// Handle case where no inspection response is received
void HandleNoInspectionResponse()
{
    IView* current_view = GetCurrentView();
    InspectorView* inspector = dynamic_cast<InspectorView*>(current_view);
    
    if (inspector)
    {
        inspector->ClearTree();
        inspector->AddLine("No client connected");
        LogInfo("Updated inspector with no-client message");
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
        g_editor.log_view.AddMessage(formatted_message);
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
        g_editor.log_view.AddMessage(message);
        render = true;
    }

    if (render)
        RequestRender();
}

static void DrawStatusBar(int width, int height)
{
    SetColor(TERM_COLOR_STATUS_BAR);

    static std::string title = "NoZ Editor";
    static std::string cmd_mode = " - Command Mode";

    MoveCursor(height - 2, 0);
    AddString(title.c_str());
    if (g_editor.command_mode)
        AddString(cmd_mode.c_str());

    AddChar(' ', width - GetCursorX() + 1);

    UnsetColor(TERM_COLOR_STATUS_BAR);
}

static void DrawCommandLine(int width, int height)
{
    SetColor(TERM_COLOR_COMMAND_LINE);

    MoveCursor(height - 1, 0);
    if (g_editor.search_mode)
    {
        AddChar('/');
        Draw(g_editor.search_input);
    }
    else if (g_editor.command_mode)
    {
        AddChar(':');
        Draw(g_editor.command_input);
    }
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

    UnsetColor(TERM_COLOR_COMMAND_LINE);
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
        g_editor.log_view.Clear();
    }
    else if (command == "i" || command == "inspector")
    {
        // Check if current view is already an inspector
        auto* current_view = GetCurrentView();
        auto* existing_inspector = dynamic_cast<InspectorView*>(current_view);
        
        if (existing_inspector)
        {
            // Reset existing inspector and request new data
            LogInfo("Resetting existing inspector view");
            existing_inspector->ClearTree();
            existing_inspector->AddLine("Waiting for client...");
            existing_inspector->ResetRequestState(); // Reset so it will send a new request
        }
        else
        {
            // Push a new inspector view onto the stack
            LogInfo("Creating new inspector view");
            InspectorView* inspector_view = new InspectorView();
            PushView(inspector_view);
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
                IView* current_view = GetCurrentView();
                current_view->SetCursorVisible(true);
            }
            else if (key == 27)
            { // Escape
                // Cancel search and return to full unfiltered view
                g_editor.search_mode = false;
                SetActive(g_editor.search_input, false);
                Clear(g_editor.search_input);
                
                IView* current_view = GetCurrentView();
                current_view->ClearSearch();
                current_view->SetCursorVisible(true);
            }
            else
            {
                HandleKey(g_editor.search_input, key);
                
                // Update search in real-time
                std::string pattern = GetText(g_editor.search_input);
                IView* current_view = GetCurrentView();
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
                IView* current_view = GetCurrentView();
                current_view->SetCursorVisible(true);
            }
            else if (key == 27)
            { // Escape
                g_editor.command_mode = false;
                SetActive(g_editor.command_input, false);
                Clear(g_editor.command_input);
                
                // Show cursor in current view when exiting command mode
                IView* current_view = GetCurrentView();
                current_view->SetCursorVisible(true);
            }
            else
            {
                HandleKey(g_editor.command_input, key);
            }
        }
        else
        {
            IView* current_view = GetCurrentView();
            
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
                    IView* current_view = GetCurrentView();
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


void RenderEditor(int width, int height)
{
    ClearScreen();
    
    DrawStatusBar(width, height);
    DrawCommandLine(width, height);
    
    // Always render the log view as the base layer
    g_editor.log_view.Render(width, height);
    
    // If there are views on the stack, render the top one instead
    if (!g_editor.view_stack.empty())
    {
        IView* current_view = g_editor.view_stack.top();
        current_view->Render(width, height);
    }
    
    // Hide the terminal cursor when not in command/search mode since views handle their own cursor display
    if (!g_editor.command_mode && !g_editor.search_mode)
    {
        SetCursorVisible(false);
    }
}

void InitEditor()
{
    g_scratch_allocator = CreateArenaAllocator(32 * noz::MB, "scratch");

    InitLog(HandleLog);
    InitTerminal();
    SetRenderCallback([](int width, int height) { RenderEditor(width, height); });
    SetResizeCallback([](int new_width, int new_height) { HandleResize(new_width, new_height); });
    int term_height = GetTerminalHeight();
    int term_width = GetTerminalWidth();
    g_editor.command_input = CreateTextInput(1, term_height - 1, term_width - 1);
    g_editor.search_input = CreateTextInput(1, term_height - 1, term_width - 1);

    // Initialize log view cursor visibility (show cursor when not in command mode)
    g_editor.log_view.SetCursorVisible(true);

    LogWarning("test warning");
    LogError("test error");
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

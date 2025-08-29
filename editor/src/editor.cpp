//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "tui/terminal.h"
#include "tui/text_input.h"
#include "views/log_view.h"

bool InitImporter();
void ShutdownImporter();
bool IsImporterRunning();

struct Editor
{
    LogView log_view;
    TextInput* command_input;
    bool command_mode = false;
    std::atomic<bool> is_running = true;
};

static Editor g_editor = {};

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
    if (g_editor.command_mode)
    {
        AddChar(':');
        Draw(g_editor.command_input);
    }
    else
    {
        for (int i = 0; i < width; i++)
            AddChar(' ');
    }

    UnsetColor(TERM_COLOR_COMMAND_LINE);
}


static void HandleCommand(const std::string& command)
{
    if (command == "q" || command == "quit")
    {
        g_editor.is_running = false;
    }
    else if (command == "clear")
    {
        g_editor.log_view.Clear();
    }
    else if (!command.empty())
    {
        LogInfo("Unknown command: %s", command.c_str());
        LogInfo("Available commands: clear, quit");
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

        if (g_editor.command_mode)
        {
            if (key == '\n' || key == '\r')
            {
                std::string command = GetText(g_editor.command_input);
                HandleCommand(command);
                g_editor.command_mode = false;
                SetActive(g_editor.command_input, false);
                Clear(g_editor.command_input);
                SetCursorVisible(false);
            }
            else if (key == 27)
            { // Escape
                g_editor.command_mode = false;
                SetActive(g_editor.command_input, false);
                Clear(g_editor.command_input);
                SetCursorVisible(false);
            }
            else
            {
                HandleKey(g_editor.command_input, key);
            }
        }
        else
        {
            if (key == ':')
            {
                g_editor.command_mode = true;
                SetActive(g_editor.command_input, true);
                Clear(g_editor.command_input);
                SetCursorVisible(true);
            }
            else if (key == 'q')
            {
                g_editor.is_running = false;
            }
        }

        RenderTerminal();
    }
}


void RenderEditor(int width, int height)
{
    ClearScreen();
    g_editor.log_view.Render(width, height);
    DrawStatusBar(width, height);
    DrawCommandLine(width, height);
}

void InitEditor()
{
    InitLog(HandleLog);
    InitTerminal();
    SetRenderCallback([](int width, int height) { RenderEditor(width, height); });
    SetResizeCallback([](int new_width, int new_height) { HandleResize(new_width, new_height); });
    int term_height = GetTerminalHeight();
    int term_width = GetTerminalWidth();
    g_editor.command_input = CreateTextInput(1, term_height - 1, term_width - 1);


    LogWarning("test warning");
    LogError("test error");
}

void ShutdownEditor()
{
    Destroy(g_editor.command_input);
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

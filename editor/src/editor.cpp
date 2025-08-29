#include "controls/text_input.h"
#include "views/log_view.h"
#include "window.h"

int InitImporter();
void ShutdownImporter();

class Editor;

static Editor* g_editor_instance = nullptr;

static FILE* g_debug_log = nullptr;

void editor_log_callback(LogType type, const char* message);

class Editor
{
    LogView _log_view;
    std::unique_ptr<text_input> _command_input;
    std::unique_ptr<Window> _window;
    bool _command_mode = false;
    std::atomic<bool> _running = true;
    std::unique_ptr<std::thread> _importer_thread;
    std::atomic<bool> _importer_running = false;

public:

    ~Editor()
    {
        if (_importer_running)
        {
            StopImporter();
        }

        _window.reset();
        g_editor_instance = nullptr;

        // Close debug log file
        if (g_debug_log)
        {
            fclose(g_debug_log);
            g_debug_log = nullptr;
        }
    }

    bool Initialize()
    {
        _window = std::make_unique<Window>();
        if (!_window->Initialize())
        {
            return false;
        }

        _window->SetRenderCallback([this](int width, int height) { this->RenderContent(width, height); });

        _window->SetResizeCallback([this](int new_width, int new_height) { this->OnResize(new_width, new_height); });

        _command_input = std::make_unique<text_input>(1, _window->GetHeight() - 1, _window->GetWidth() - 1);

        return true;
    }

    void HandleLogMessage(const std::string& message)
    {
        _log_view.AddMessage(message);
        _window->RequestRedraw();
    }

    void OnResize(int new_width, int new_height)
    {
        _command_input = std::make_unique<text_input>(1, new_height - 1, new_width - 1);
    }

    void RenderContent(int width, int height)
    {
        _window->ClearScreen();
        _log_view.Render(_window.get(), width, height);
        DrawStatusBar(width, height);
        DrawCommandLine(width, height);
    }

    void DrawStatusBar(int width, int height)
    {
        _window->SetColor(Window::COLOR_STATUS_BAR);

        std::string status = "NoZ Editor";
        if (_command_mode)
        {
            status += " - Command Mode";
        }

        _window->MoveCursor(height - 2, 0);
        for (int i = 0; i < width; i++)
        {
            char ch = (i < static_cast<int>(status.length())) ? status[i] : ' ';
            _window->AddChar(ch);
        }

        _window->UnsetColor(Window::COLOR_STATUS_BAR);
    }

    void DrawCommandLine(int width, int height)
    {
        _window->SetColor(Window::COLOR_COMMAND_LINE);

        _window->MoveCursor(height - 1, 0);
        if (_command_mode)
        {
            _window->AddChar(':');

            std::string input_text = _command_input->get_text();
            size_t cursor_pos = _command_input->get_cursor_pos();
            int available_width = width - 1;

            // Handle scrolling for long text
            std::string display_text = input_text;
            size_t display_cursor_pos = cursor_pos;

            if (input_text.length() > static_cast<size_t>(available_width))
            {
                if (cursor_pos >= static_cast<size_t>(available_width))
                {
                    // Scroll text to keep cursor visible
                    size_t start = cursor_pos - available_width + 1;
                    display_text = input_text.substr(start);
                    display_cursor_pos = available_width - 1;
                }
                else
                {
                    display_text = input_text.substr(0, available_width);
                }
            }

            _window->AddString(display_text.c_str());

            int remaining = available_width - static_cast<int>(display_text.length());
            for (int i = 0; i < remaining; i++)
            {
                _window->AddChar(' ');
            }

            int final_cursor_x = 1 + static_cast<int>(display_cursor_pos);
            _window->MoveCursor(height - 1, final_cursor_x);
        }
        else
        {
            for (int i = 0; i < width; i++)
            {
                _window->AddChar(' ');
            }
            _window->MoveCursor(0, 0);
        }

        _window->UnsetColor(Window::COLOR_COMMAND_LINE);
    }

    void StartImporter()
    {
        if (_importer_running)
        {
            LogInfo("Importer is already running");
            return;
        }

        LogInfo("Starting asset importer...");
        _importer_running = true;

        _importer_thread = std::make_unique<std::thread>(
            [this]()
            {
                int result = InitImporter();
                _importer_running = false;
                LogInfo("Importer stopped with code: %d", result);
            });
    }

    void StopImporter()
    {
        if (!_importer_running)
        {
            LogInfo("Importer is not running");
            return;
        }

        LogInfo("Stopping importer...");
        ShutdownImporter();

        if (_importer_thread && _importer_thread->joinable())
        {
            _importer_thread->join();
        }
        _importer_thread.reset();
    }

    void HandleCommand(const std::string& command)
    {
        if (command == "q" || command == "quit")
        {
            _running = false;
        }
        else if (command == "start" || command == "import")
        {
            StartImporter();
        }
        else if (command == "stop")
        {
            StopImporter();
        }
        else if (command == "restart")
        {
            StopImporter();
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Small delay to ensure clean shutdown
            StartImporter();
        }
        else if (command == "clear")
        {
            _log_view.Clear();
        }
        else if (command == "status")
        {
            if (_importer_running)
            {
                LogInfo("Importer is running");
            }
            else
            {
                LogInfo("Importer is stopped");
            }
        }
        else if (!command.empty())
        {
            LogInfo("Unknown command: %s", command.c_str());
            LogInfo("Available commands: start, stop, restart, status, clear, quit");
        }
    }

    void Run()
    {
        if (!Initialize())
        {
            std::cerr << "Failed to initialize ncurses\n";
            return;
        }

        g_editor_instance = this;
        LogInit(editor_log_callback);

        LogInfo("NoZ Editor starting...");
        LogInfo("Auto-starting asset importer...");

        StartImporter();
        _window->Render();

        while (_running)
        {
            // Handle resize with highest priority
            if (_window->ShouldResize())
            {
                _window->HandleResize();
            }

            _window->CheckResize();
            int key = _window->GetKey();

            if (key == ERR)
            {
                if (_window->NeedsRedraw())
                {
                    _window->Render();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }

            if (_command_mode)
            {
                if (key == '\n' || key == '\r')
                {
                    std::string command = _command_input->get_text();
                    HandleCommand(command);
                    _command_mode = false;
                    _command_input->set_active(false);
                    _command_input->clear();
                    _window->SetCursorVisible(false);
                }
                else if (key == 27)
                { // Escape
                    _command_mode = false;
                    _command_input->set_active(false);
                    _command_input->clear();
                    _window->SetCursorVisible(false);
                }
                else
                {
                    _command_input->handle_key(key);
                }
            }
            else
            {
                if (key == ':')
                {
                    _command_mode = true;
                    _command_input->set_active(true);
                    _command_input->clear();
                    _window->SetCursorVisible(true);
                }
                else if (key == 'q')
                {
                    _running = false;
                }
            }

            _window->Render();
        }
    }
};

void editor_log_callback(LogType type, const char* message)
{
    // Handle debug logging to file
    if (type == LOG_TYPE_DEBUG)
    {
        if (!g_debug_log)
        {
            g_debug_log = fopen("debug.log", "w");
        }
        if (g_debug_log)
        {
            fprintf(g_debug_log, "[DEBUG] %s\n", message);
            fflush(g_debug_log);
        }
        return; // Don't show debug messages in editor UI
    }
    
    // Route other messages to editor UI
    if (g_editor_instance)
    {
        // Add type prefix for display
        std::string formatted_message;
        switch(type) {
            case LOG_TYPE_INFO: formatted_message = "[INFO] " + std::string(message); break;
            case LOG_TYPE_WARNING: formatted_message = "[WARNING] " + std::string(message); break;
            case LOG_TYPE_ERROR: formatted_message = "[ERROR] " + std::string(message); break;
            default: formatted_message = std::string(message); break;
        }
        g_editor_instance->HandleLogMessage(formatted_message);
    }
}

int main(int argc, char* argv[])
{
    Editor editor;
    editor.Run();
    return 0;
}

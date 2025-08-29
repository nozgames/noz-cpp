#pragma once

#include "terminal.h"
#include <thread>

class WindowsTerminal : public Terminal {
public:
    WindowsTerminal();
    ~WindowsTerminal() override;

    bool Initialize() override;
    void Cleanup() override;
    void HandleResize() override;
    void CheckResize() override;
    bool ShouldResize() override;

private:
    std::thread _resize_thread;
    std::atomic<bool> _resize_thread_running = false;
    void ResizeThreadLoop();
};
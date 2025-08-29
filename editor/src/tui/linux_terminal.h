#pragma once

#include "terminal.h"

class LinuxTerminal : public Terminal {
public:
    LinuxTerminal();
    ~LinuxTerminal() override;

    bool Initialize() override;
    void Cleanup() override;
    void HandleResize() override;
    void CheckResize() override;
    bool ShouldResize() override;
};
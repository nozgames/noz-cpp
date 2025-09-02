//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#ifdef _WIN32

#include <noz/noz.h>
#include <noz/platform.h>
#include <windows.h>
#include <direct.h>

#define WIN32_LEAN_AND_MEAN
#include <shellapi.h>

void thread_sleep_ms(int milliseconds)
{
    Sleep(milliseconds);
}

extern int main(int argc, char* argv[]);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Convert command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 1;

    char* args[1] = { (char*)".exe" };
    return main(1, args);
}

#endif

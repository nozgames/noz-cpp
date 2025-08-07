#ifdef _WIN32

#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

// Forward declaration of main function
extern int main(int argc, char* argv[]);

// Custom streambuf to redirect output to OutputDebugString
class DebugStreamBuf : public std::streambuf 
{
private:
    std::string buffer;

protected:
    
	virtual int_type overflow(int_type c) override 
	{
        if (c != EOF) {
            buffer += static_cast<char>(c);
            if (c == '\n') {
                OutputDebugStringA(buffer.c_str());
                buffer.clear();
            }
        }
        return c;
    }

    virtual int sync() override {
        if (!buffer.empty()) {
            OutputDebugStringA(buffer.c_str());
            buffer.clear();
        }
        return 0;
    }
};

// Global debug stream buffer
static DebugStreamBuf debugStreamBuf;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Redirect cout and cerr to Visual Studio debug window
    std::cout.rdbuf(&debugStreamBuf);
    std::cerr.rdbuf(&debugStreamBuf);
    
    // Also redirect to a console window for debugging
    AllocConsole();
    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);

    // Convert command line arguments
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
    {
        return 1;
    }

    // Convert wide strings to regular strings
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++)
    {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, &strTo[0], size_needed, NULL, NULL);
        args.push_back(strTo);
    }
    LocalFree(argv);

    // Convert to char* array for compatibility
    std::vector<char*> charArgs;
    for (auto& arg : args)
    {
        charArgs.push_back(&arg[0]);
    }

    // Call the regular main function
    return main(static_cast<int>(charArgs.size()), charArgs.data());
}

#endif 

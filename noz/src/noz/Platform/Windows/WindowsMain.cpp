///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Text/StringLexer.h>
#include <noz/IO/StringReader.h>

#include "Windows.pch.h"
#include "WindowsWindow.h"

using namespace noz;
using namespace noz::Platform::Windows;

#define ENABLE_VISUAL_LEAK_DETECTOR   0
#define ENABLE_CRT_DEBUG              0
#define ENABLE_MAX_FPS                0

namespace noz { namespace Platform { 
  noz_int32 GetOpenGLShaderVersion (void) {return 150;}
}}

namespace noz { 
namespace Platform { 
namespace Windows {

  static ATOM RegisterClass (HINSTANCE hInstance) {
	  WNDCLASSEX wcex;
	  wcex.cbSize = sizeof(WNDCLASSEX);

	  wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	  wcex.lpfnWndProc	= WindowsWindow::WndProc;
	  wcex.cbClsExtra		= 0;
	  wcex.cbWndExtra		= 0;
	  wcex.hInstance		= hInstance;
	  wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(1));
	  wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	  wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	  wcex.lpszMenuName	= 0;;
	  wcex.lpszClassName	= "noz::Platform::WindowsWindow";
	  wcex.hIconSm		= LoadIcon(hInstance, MAKEINTRESOURCE(1));

	  return RegisterClassEx(&wcex);
  }

}}}

/*
void noz::Application::Run (void) {
}

void noz::Application::Run (Window* _window) {
  ObjectPtr<Window> window = _window;

	// Main message loop:
	MSG msg;

  // Check to see if any messages are waiting in the queue
  while(window && !window->IsClosed()) {
    while(window && !window->IsClosed() && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      // If the message is WM_QUIT, exit the while loop
      if(msg.message == WM_QUIT)
        break;

      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // If the message is WM_QUIT, exit the while loop
    if(msg.message == WM_QUIT) break;

#if ENABLE_MAX_FPS
  	Sleep(1.0f / 120.0f);
#endif

    // Execute a single frame.
    Application::Frame();
  }   
}
*/


#if ENABLE_VISUAL_LEAK_DETECTOR
#include <vld.h>
#endif

int APIENTRY WinMain(
  _In_ HINSTANCE hInstance,
  _In_opt_ HINSTANCE hPrevInstance,
  _In_ LPTSTR lpCmdLine,
  _In_ int nCmdShow) {

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

  // Initialize windows sockets
  WSADATA wsadata;
  int error = WSAStartup(0x0202, &wsadata);

#if ENABLE_CRT_DEBUG
  // Get current flag
  int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

  // Turn on leak-checking bit.
  tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

  // Turn off CRT block checking bit.
  //tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;
  tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF ;

  // Set flag to the new value.
  _CrtSetDbgFlag( tmpFlag );
#endif

  // Cache hInstance
  WindowsWindow::hinstance_ = hInstance;

  OleInitialize(NULL);

	// Initialize global strings
	RegisterClass(hInstance);  

  char filename[MAX_PATH];
  GetModuleFileName(hInstance, filename, MAX_PATH);

  std::vector<String> args;
  const char* cmd = GetCommandLine();
  if(cmd && *cmd) {
    StringReader reader(cmd);
    StringLexer lexer(&reader,"",StringLexer::FlagLiteralStrings);
    while(!lexer.IsEnd()) {
      args.push_back(lexer.Consume());
    }  
  }

  const char** argv = new const char*[args.size()];
  for(noz_uint32 i=0;i<args.size();i++) {
    argv[i] = args[i].ToCString();
  }

  Application::Initialize(args.size(),argv);

	// Main message loop:
	MSG msg;

  // Check to see if any messages are waiting in the queue
  ObjectPtr<Window> window = Window::GetPrimaryWindow();
  while(window && !window->IsClosed()) {
    while(window && !window->IsClosed() && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      // If the message is WM_QUIT, exit the while loop
      if(msg.message == WM_QUIT)
        break;

      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // If the message is WM_QUIT, exit the while loop
    if(msg.message == WM_QUIT) break;

#if ENABLE_MAX_FPS
  	Sleep(1.0f / 120.0f);
#endif

    // Execute a single frame.
    Application::Frame();
  }   

  Application::Uninitialize();

  delete argv;
  args.clear();

  return 0;
}

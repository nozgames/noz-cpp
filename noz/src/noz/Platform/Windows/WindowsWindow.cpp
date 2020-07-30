///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/UI/SystemCursor.h>
#include <noz/Serialization/BinarySerializer.h>
#include <noz/Serialization/BinaryDeserializer.h>
#include <noz/IO/MemoryStream.h>
#include <noz/Platform/CursorHandle.h>

#include "Windows.pch.h"
#include "WindowsWindow.h"
#include "WindowsDragDrop.h"

using namespace noz;
using namespace noz::Platform;
using namespace noz::Platform::Windows;

#if 0
#define NVPM_INITGUID
#include "NvPmApi.Manager.h"

// Simple singleton implementation for grabbing the NvPmApi static NvPmApiManager S_NVPMManager;
static NvPmApiManager S_NVPMManager;
extern NvPmApiManager *GetNvPmApiManager() {return &S_NVPMManager;}
const NvPmApi *GetNvPmApi() {return S_NVPMManager.Api();}
#define PATH_TO_NVPMAPI_CORE  L"C:\\Users\\Bryan\\Desktop\\PerfKit\\bin\\x86\\NvPmApi.Core.dll"
#endif

HINSTANCE WindowsWindow::hinstance_ = NULL;
WindowsWindow* WindowsWindow::primary_ = nullptr;
std::vector<CachedWindow*> WindowsWindow::cache_;


namespace noz {
namespace Platform {

  class WindowsCursorHandle : public CursorHandle {
    public: HCURSOR hcursor_;
    public: bool default_;

    public: WindowsCursorHandle(Cursor* cursor) {
      default_ = false;
      if(cursor->IsTypeOf(typeof(SystemCursor))) {
        switch(((SystemCursor*)cursor)->GetCursorType()) {
          case SystemCursorType::Default:
          case SystemCursorType::Arrow:
            hcursor_ = ::LoadCursor(NULL,IDC_ARROW);
            default_ = true;
            break;

          case SystemCursorType::Cross:
            hcursor_ = ::LoadCursor(NULL,IDC_CROSS);
            break;            

          case SystemCursorType::SizeWE:
            hcursor_ = ::LoadCursor(NULL,IDC_SIZEWE);
            break;            

          case SystemCursorType::SizeNS:
            hcursor_ = ::LoadCursor(NULL,IDC_SIZENS);
            break;            

          case SystemCursorType::SizeNWSE:
            hcursor_ = ::LoadCursor(NULL,IDC_SIZENWSE);
            break;            

          case SystemCursorType::SizeNESW:
            hcursor_ = ::LoadCursor(NULL,IDC_SIZENESW);
            break;            

          case SystemCursorType::SizeAll:
            hcursor_ = ::LoadCursor(NULL,IDC_SIZEALL);
            break;            

          case SystemCursorType::IBeam:
            hcursor_ = ::LoadCursor(NULL,IDC_IBEAM);
            break;            

          case SystemCursorType::Hand:
            hcursor_ = ::LoadCursor(NULL,IDC_HAND);
            break;            
        }
      }
    }
  };

}
}

WindowHandle* WindowHandle::New (Window* window, WindowAttributes attr) {
  return new WindowsWindow(window,attr);
}

WindowsWindow::WindowsWindow(Window* window, WindowAttributes attr) {
  // Intialize drag drop system.
  WindowsDragDrop::Initialize();

  window_ = window;
  is_tracking_= false;
  cursor_ = NULL;

  DWORD style = 0;
  DWORD exstyle = 0;
  if(attr & WindowAttributes::Popup) {
    style = WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_POPUP;
    exstyle = WS_EX_NOACTIVATE;
    is_popup_ = true;
  } else {    
    if(attr & WindowAttributes::Primary) {
      style = WS_CLIPSIBLINGS|WS_CAPTION|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU|WS_SIZEBOX;
    } else {
      style = WS_CLIPSIBLINGS|WS_POPUP|WS_CAPTION; // |WS_SYSMENU;
      exstyle = WS_EX_TOOLWINDOW; // WS_EX_DLGMODALFRAME;
    }
    is_popup_ = false;
  }

  WindowsWindow* parent = nullptr;
  if(window->GetParent()) {
    parent = ((WindowsWindow*)window->GetParent()->GetHandle());
  }
  
  if(cache_.empty()) {
    cached_ = new CachedWindow;
    cached_->hwnd = CreateWindowEx(exstyle,"noz::Platform::WindowsWindow", "", style, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, parent?parent->cached_->hwnd:NULL, NULL, WindowsWindow::hinstance_, (LPVOID)this);  
    InitOpenGL(parent);
  } else {
    cached_ = cache_.back();
    cache_.pop_back();

    SetWindowLong(cached_->hwnd,GWL_STYLE,style);
    SetWindowLong(cached_->hwnd,GWL_EXSTYLE,exstyle);
    SetWindowLong(cached_->hwnd,GWL_USERDATA,(LONG)this);
  }

  // Drag-Drop support.
  WindowsDragDrop::DropTargetCallbacks callbacks;
  ZeroMemory(&callbacks,sizeof(callbacks));
  callbacks.drag_enter_ = WindowsWindow::OnDragEnter; 
  callbacks.drag_over_ = WindowsWindow::OnDragOver; 
  callbacks.drag_leave_ = WindowsWindow::OnDragLeave;
  callbacks.drop_ = WindowsWindow::OnDrop;
  drop_target_ = WindowsDragDrop::CreateDropTarget(&callbacks,window);
  RegisterDragDrop(cached_->hwnd, (IDropTarget*)drop_target_);


  if(attr&WindowAttributes::Primary) {
    primary_ = this;
  
    // Create a window for popup cache..
    //delete (new WindowsWindow(window_, WindowAttributes::Popup));

  } else {
//    Move(0,0);
  }

  //SetFocus(cached_->hwnd);
}

WindowsWindow::~WindowsWindow(void) {
  if(drop_target_) RevokeDragDrop(cached_->hwnd);
  SetWindowLong(cached_->hwnd,GWL_USERDATA,0);
  ShowWindow(cached_->hwnd,SW_HIDE);
  cache_.push_back(cached_);
  ReleaseCapture();
}

void WindowsWindow::SetCursor(Cursor* cursor) {
  noz_assert(cursor);
  noz_assert(cursor->GetHandle());

  if(nullptr != drag_drop_object_) return;  

  if(!((WindowsCursorHandle*)cursor->GetHandle())->default_) {
    if(cursor_ != ((WindowsCursorHandle*)cursor->GetHandle())->hcursor_) {
      cursor_ = ((WindowsCursorHandle*)cursor->GetHandle())->hcursor_;
      SendMessage(cached_->hwnd, WM_SETCURSOR, 0, 0);
    }
  } else if(cursor_ != NULL) {
    ::SetCursor(((WindowsCursorHandle*)cursor->GetHandle())->hcursor_);
    cursor_ = NULL;
  }
}

void WindowsWindow::SetCapture(void) {
  ::SetCapture(cached_->hwnd);
}

void WindowsWindow::SetFocus (void) {
  noz_assert(cached_);
  noz_assert(cached_->hwnd);
  ::SetFocus(cached_->hwnd);
}

void WindowsWindow::ReleaseCapture(void) {
  ::ReleaseCapture();
}



noz_uint32 WindowsWindow::GetModifiers (WPARAM wparam) {
  noz_uint32 modifiers = 0;
  if(wparam & MK_CONTROL) modifiers |= Keys::Control;
  if(wparam & MK_SHIFT) modifiers |= Keys::Shift;
  if(wparam & MK_ALT) modifiers |= Keys::Alt;
  return modifiers;
}

noz_uint32 WindowsWindow::GetKeyData(char c) {
  static noz_uint32 lut[256];
  static bool initialied = false;
  if(!initialied) {
    initialied = true;
    int i;
    for(i='A';i<='Z';i++) lut[i] = (Keys::A + i-'A') + Keys::Shift;
    for(i='a';i<='z';i++) lut[i] = (Keys::A + i-'a');
    for(i='0';i<='9';i++) lut[i] = Keys::D0 + i-'0';
    lut['-'] = Keys::OemMinus;
    lut[' '] = Keys::Space;
    lut['.'] = Keys::OemPeriod;
    lut[','] = Keys::OemComma;
    lut[';'] = Keys::OemSemicolon;
    lut[':'] = Keys::OemSemicolon + Keys::Shift;
    lut['\r'] = Keys::Return;
  }

  noz_uint32 result = lut[(noz_byte)c];
  if(result == 0) {
    return 0;
  }

  if((GetKeyState(VK_SHIFT) & 0x8000) == 0x8000) {
    result |= Keys::Shift;
  }

  if((GetKeyState(VK_CONTROL) & 0x8000) == 0x8000) {
    result |= Keys::Control;
  }

  return result;
}

noz_uint32 WindowsWindow::GetKeyData(WPARAM wparam) {
  if(wparam==0) return 0;

  static noz_uint32 lut[256];
  static bool initialied = false;
  if(!initialied) {
    initialied = true;
    int i;
    for(i='A';i<='Z';i++) lut[i] = Keys::A + i-'A';
    for(i='a';i<='z';i++) lut[i] = Keys::A + i-'a';
    for(i='0';i<='9';i++) lut[i] = Keys::D0 + i-'0';
    lut[VK_OEM_1] = Keys::OemSemicolon;
    lut[VK_SPACE] = Keys::Space;
    lut[VK_BACK] = Keys::Back;
    lut[VK_DELETE] = Keys::Delete;
    lut[VK_TAB] = Keys::Tab;
    lut[VK_HOME] = Keys::Home;
    lut[VK_END] = Keys::End;
    lut[VK_LEFT] = Keys::Left;
    lut[VK_RIGHT] = Keys::Right;
    lut[VK_UP] = Keys::Up;
    lut[VK_DOWN] = Keys::Down;
    lut[VK_OEM_PERIOD] = Keys::OemPeriod;
    lut[VK_OEM_COMMA] = Keys::OemComma;
    lut[VK_OEM_MINUS] = Keys::OemMinus;
    lut[VK_RETURN] = Keys::Return;
    lut[VK_ESCAPE] = Keys::Escape;
    lut[VK_F1] = Keys::F1;
    lut[VK_F2] = Keys::F2;
    lut[VK_F3] = Keys::F3;
    lut[VK_F4] = Keys::F4;
    lut[VK_F5] = Keys::F5;
    lut[VK_F6] = Keys::F6;
    lut[VK_F7] = Keys::F7;
    lut[VK_F8] = Keys::F8;
    lut[VK_F9] = Keys::F9;
    lut[VK_F10] = Keys::F10;
    lut[VK_F11] = Keys::F11;
    lut[VK_F12] = Keys::F12;
  }

  noz_uint32 result = lut[wparam];

  // Determine if shift state..
  //if(((GetKeyState(VK_CAPITAL) & 0x0001) == 1) || ((GetKeyState(VK_SHIFT) & 0x8000) == 0x8000)) {
  if((GetKeyState(VK_SHIFT) & 0x8000) == 0x8000) {
    result |= Keys::Shift;
  }

  if((GetKeyState(VK_CONTROL) & 0x8000) == 0x8000) {
    result |= Keys::Control;
  }

  return result;
}

LRESULT CALLBACK WindowsWindow::WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  // Retrieve the WinWindow pointer from the windows user data
  WindowsWindow* win=(WindowsWindow*)GetWindowLong(hwnd,GWL_USERDATA);  
  if(nullptr==win) {
    // Special case for create
    if(message == WM_CREATE) {
      LPCREATESTRUCT lpcs=(LPCREATESTRUCT)lparam;
      SetWindowLong(hwnd,GWL_USERDATA,(LONG)lpcs->lpCreateParams);
      return 0;
    }

    return DefWindowProc(hwnd, message, wparam, lparam);
  }

	switch (message) {
    case WM_ACTIVATE:
      if(wparam == WA_INACTIVE && (HWND)lparam == hwnd) {
//        Application::PostEvent(new FocusLostEvent(win->GetWindow()));
      }
      if(wparam != WA_INACTIVE && win->window_->IsPopup()) return 0;
      break;

    case WM_NCACTIVATE:
      if(wparam != WA_INACTIVE && win->window_->IsPopup()) {
        for(Window* p = win->window_->GetParent(); p; p=p->GetParent()) {
          if(!p->IsPopup()) {
            SendMessage (((WindowsWindow*)p->GetHandle())->cached_->hwnd, WM_NCACTIVATE, WA_ACTIVE, 0);
            return 0;
          }
        }
      }
      break;

      /*
    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
//      Application::PostEvent(new FocusLostEvent(win->GetWindow()));
      break;
      */

    case WM_CAPTURECHANGED:
      SystemEvent::PushEvent(SystemEventType::CaptureChanged,win->GetWindow());
      break;

    case WM_CLOSE: 
      if(win->GetWindow()->IsPrimary())  {        
        PostQuitMessage(0); break;
      } else {
        delete win->GetWindow();
      }
      break;

    case WM_KILLFOCUS:
      //Application::PostEvent(new FocusLostEvent(win->GetWindow()));
      break;

    case WM_MOVE:
      win->GetWindow()->OnPositionChanged ();
      break;

    case WM_SIZE:
      if(!IsIconic(hwnd)) win->GetWindow()->OnSizeChanged ();
      break;

    case WM_SETCURSOR:
      if(win->drag_drop_object_!=nullptr) {
        return TRUE;
      }
      
      if(win->cursor_) {
        ::SetCursor(win->cursor_);
        return TRUE;
      }

      return DefWindowProc(hwnd, message, wparam, lparam);

    case WM_PAINT: {
      PAINTSTRUCT ps;
      ::BeginPaint(hwnd, &ps); 
      //win->Paint();
      ::EndPaint(hwnd,&ps);
      break;
    }

    case WM_MOUSEWHEEL: {
      POINT pt;
      pt.x = GET_X_LPARAM(lparam);
      pt.y = GET_Y_LPARAM(lparam);
      ScreenToClient(hwnd,&pt);
      SystemEvent::PushEvent(SystemEventType::MouseWheel,win->window_,Vector2((noz_float)pt.x,(noz_float)pt.y),Vector2(0.0f,((noz_float)GET_WHEEL_DELTA_WPARAM(wparam)) / 120.0f),win->GetKeyData((WPARAM)0));
      break;
    }

    case WM_ERASEBKGND:
      return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
      SystemEvent::PushEvent(SystemEventType::KeyDown, win->window_, win->GetKeyData(wparam));
      break;

    case WM_SYSKEYUP:
    case WM_KEYUP:
      SystemEvent::PushEvent(SystemEventType::KeyUp, win->window_, win->GetKeyData(wparam));
      break;

    case WM_SYSCHAR:
    case WM_CHAR: {
      noz_uint32 kd = win->GetKeyData((char)wparam);
      if(kd != 0) {
        //Application::PostEvent(new KeyEvent(win->GetWindow(),KeyEvent::KeyState::Press, kd));
      }
      break;
    }

    case WM_LBUTTONDBLCLK:
      SystemEvent::PushEvent(SystemEventType::MouseDown,win->window_,MouseButton::Left,2,win->GetPosition(lparam),win->GetKeyData(wparam));
      return 0;

    case WM_LBUTTONDOWN:
      SystemEvent::PushEvent(SystemEventType::MouseDown,win->window_,MouseButton::Left,1,win->GetPosition(lparam),win->GetKeyData(wparam));
      return 0;

    case WM_LBUTTONUP:
      SystemEvent::PushEvent(SystemEventType::MouseUp,win->window_,MouseButton::Left,1,win->GetPosition(lparam),win->GetKeyData(wparam));
      return 0;

    case WM_RBUTTONDBLCLK:
      SystemEvent::PushEvent(SystemEventType::MouseDown,win->window_,MouseButton::Right,2,win->GetPosition(lparam),win->GetKeyData(wparam));
      return 0;
      
    case WM_RBUTTONDOWN:
      SystemEvent::PushEvent(SystemEventType::MouseDown,win->window_,MouseButton::Right,1,win->GetPosition(lparam),win->GetKeyData(wparam));
      return 0;

    case WM_RBUTTONUP:
      SystemEvent::PushEvent(SystemEventType::MouseUp,win->window_,MouseButton::Right,1,win->GetPosition(lparam),win->GetKeyData(wparam));
      return 0;

    case WM_MOUSEMOVE:
      SystemEvent::PushEvent(SystemEventType::MouseMove,win->window_,win->GetPosition(lparam),Vector2::Zero, win->GetKeyData(wparam));
      break;

	  default: break;		  
  }

  return DefWindowProc(hwnd, message, wparam, lparam);
}


void WindowsWindow::Show(void) {
  if(is_popup_) {
    ShowWindow(cached_->hwnd,SW_SHOWNOACTIVATE);
    RedrawWindow(cached_->hwnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW);
  } else {
    ShowWindow(cached_->hwnd,SW_SHOW);
  }
}

void WindowsWindow::SetTitle(String title) {
  SetWindowText(cached_->hwnd,title.ToCString());
}

void WindowsWindow::SetSize(noz_float width, noz_float height) {
  RECT wr;
  RECT cr;
  ::GetWindowRect(cached_->hwnd,&wr);
  ::GetClientRect(cached_->hwnd,&cr);
  width += ((wr.right-wr.left)-(cr.right-cr.left));
  height += ((wr.bottom-wr.top)-(cr.bottom-cr.top));

  SetWindowPos(cached_->hwnd, NULL, 0, 0, noz_int32(width), noz_int32(height), SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
}

void WindowsWindow::Move(noz_float x, noz_float y) {
  //SetParent(cached_->hwnd, primary_->cached_->hwnd);
#if 0
  if(window_->GetParent()) {
    WindowsWindow* parent = (WindowsWindow*)window_->GetParent()->GetHandle();
    POINT p;      
    p.x = noz_int32(x);
    p.y = noz_int32(y);
    ::ClientToScreen(parent->cached_->hwnd,&p);
    SetWindowPos(cached_->hwnd, NULL, p.x, p.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
  } else {
    SetWindowPos(cached_->hwnd, NULL, noz_int32(x), noz_int32(y), 0, 0, SWP_NOMOVE|SWP_NOZORDER);
  }
#else
    SetWindowPos(cached_->hwnd, NULL, noz_int32(x), noz_int32(y), 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
#endif
}

void WindowsWindow::InitOpenGL(WindowsWindow* parent) {
	// Get a DC for the OpenGL render window
	cached_->hdc = ::GetDC(cached_->hwnd);

	// Set the pixel format for this DC
	static PIXELFORMATDESCRIPTOR pfd = {
		sizeof (PIXELFORMATDESCRIPTOR),	// struct size 
		1,						                  // Version number
		PFD_DRAW_TO_WINDOW |            // Flags, draw to a window,
		PFD_SUPPORT_OPENGL |            // use OpenGL
		PFD_DOUBLEBUFFER,		            // double buffered
		PFD_TYPE_RGBA,                  // RGBA pixel values
		32,                             // 32-bit color
		0, 0, 0,                        // RGB bits & shift sizes.
		0, 0, 0,                        // Don't care about them
		0, 0,                           // No alpha buffer info
		0, 0, 0, 0, 0,                  // No accumulation buffer
		32,                             // 32-bit depth buffer
		8,                              // 8-bit stencil buffer
		0,                              // No auxiliary buffers
		PFD_MAIN_PLANE,                 // Layer type
		0,                              // Reserved (must be 0)
		0,                              // No layer mask
		0,                              // No visible mask
		0                               // No damage mask
	};


  int nMyPixelFormatID = ChoosePixelFormat(cached_->hdc, &pfd);
  SetPixelFormat(cached_->hdc, nMyPixelFormatID, &pfd);
	cached_->hglrc = wglCreateContext(cached_->hdc);
  wglMakeCurrent (cached_->hdc, cached_->hglrc);
  
#if defined(NOZ_WINDOWS)
  static bool glew=false;
  if(!glew) {
    glewInit();
    glew=true;
  }  
#endif


#if 0
  NVPMContext hNVPMContext(0);
  if(GetNvPmApiManager()->Construct(PATH_TO_NVPMAPI_CORE) == S_OK) {
    NVPMRESULT nvResult;
    if((nvResult = GetNvPmApi()->Init()) == NVPM_OK) {
      if((nvResult = GetNvPmApi()->CreateContextFromOGLContext((APIContextHandle)cached_->hglrc, &hNVPMContext)) == NVPM_OK) {        
        Console::WriteLine("NVPerf Initialized");
      }      
    }
  }
#endif

  wglSwapIntervalEXT(0);

  if(parent) {
    wglShareLists(parent->cached_->hglrc, cached_->hglrc);
  }
}


void WindowsWindow::BeginPaint(void) {
  wglMakeCurrent (cached_->hdc, cached_->hglrc);

  glClearColor(255,0,255,1);
  glClear(GL_COLOR_BUFFER_BIT);

#if 0
  window_->UpdateStyle(true);

  wglMakeCurrent (cached_->hdc, cached_->hglrc);
  glClearColor(0,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT);

  RECT r;
  ::GetClientRect (cached_->hwnd,&r);
  
  Size s(noz_float(r.right-r.left),noz_float(r.bottom-r.top));
  gc_->Begin(s);
  window_->OnRender(gc_);  
  window_->Paint(gc_);
  gc_->End();
  SwapBuffers(cached_->hdc);  
#endif
}

void WindowsWindow::EndPaint(void) {
  SwapBuffers(cached_->hdc);  
}

Rect WindowsWindow::GetScreenRect(void) const {
  RECT r;
  ::GetWindowRect(cached_->hwnd, &r);
  return Rect((noz_float)r.left,(noz_float)r.top,(noz_float)r.right-(noz_float)r.left,(noz_float)r.bottom-(noz_float)r.top);
}

Rect WindowsWindow::GetClientRect(void) const {
  RECT r;
  ::GetClientRect(cached_->hwnd, &r);
  return Rect((noz_float)r.left,(noz_float)r.top,(noz_float)r.right,(noz_float)r.bottom);
}

Vector2 WindowsWindow::GetScreenSize (void) const {
  HMONITOR hmonitor = ::MonitorFromWindow(cached_->hwnd, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO mi;
  mi.cbSize = sizeof(mi);
  GetMonitorInfo(hmonitor,&mi);
  return Vector2(
    (noz_float)(mi.rcMonitor.right-mi.rcMonitor.left),
    (noz_float)(mi.rcMonitor.bottom-mi.rcMonitor.top));
}

Vector2 WindowsWindow::LocalToScreen (const Vector2& v) const {
  POINT pt1;
  pt1.x = (noz_int32)v.x;
  pt1.y = (noz_int32)v.y;
  ::ClientToScreen(cached_->hwnd, &pt1);
  return Vector2((noz_float)pt1.x, (noz_float)pt1.y);
}

Rect WindowsWindow::ClientToScreen(const Rect& r) const {
  POINT pt1;
  pt1.x = (noz_int32)r.x;
  pt1.y = (noz_int32)r.y;
  ::ClientToScreen(cached_->hwnd, &pt1);
  POINT pt2;
  pt2.x = (noz_int32)(r.x+r.w);
  pt2.y = (noz_int32)(r.y+r.h);
  ::ClientToScreen(cached_->hwnd, &pt2);
  return Rect((noz_float)pt1.x,(noz_float)pt1.y,(noz_float)(pt2.x-pt1.x),(noz_float)(pt2.y-pt1.y));
}

void WindowsWindow::SetInteractive (bool interactive) {
  ::EnableWindow(cached_->hwnd, interactive);
}

void WindowsWindow::Close (void) {
  DestroyWindow(cached_->hwnd);
}

CursorHandle* CursorHandle::CreateInstance(Cursor* cursor) {
  return new WindowsCursorHandle(cursor);
}



void Application::SetClipboard (Object* data) {
  if (!::OpenClipboard(NULL)) return; 

  // Empty current clipboard contents
  ::EmptyClipboard(); 

  // Clear the clipboard if nullptr is given
  if(nullptr == data) {    
    CloseClipboard();
    return;
  }
   
  // Special case to handle text to allow it to be seen by other applications
  if(data->IsTypeOf(typeof(StringObject))) {
    const String& text=((StringObject*)data)->GetValue();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.GetLength()+1);
    if(hg==NULL) {
      CloseClipboard();
      return;
    }

    char* lock=(char*)GlobalLock(hg);
    memcpy(lock,text.ToCString(),text.GetLength()+1);
    GlobalUnlock(hg);
    SetClipboardData(CF_TEXT,hg);      
  } else {
    // Register the clip format.
    UINT cf_format = ::RegisterClipboardFormat(data->GetType()->GetQualifiedName().ToCString());
    if(cf_format==0) {
      return;
    }

    MemoryStream stream(4096);
    BinarySerializer().Serialize(data,&stream);

    // Allocate the global memory for the clipboard data
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, stream.GetLength()); 
    if(hg==NULL) {
      CloseClipboard();
      return;
    }

    char* lock=(char*)GlobalLock(hg);
    memcpy (lock, stream.GetBuffer(), stream.GetLength());
    GlobalUnlock(hg);
    SetClipboardData(cf_format,hg);
  }

  CloseClipboard();
}

Object* Application::GetClipboard (Type* type) {
  UINT cf = 0;

  // Open the clipboard to read the data     
  if(!OpenClipboard(NULL)) return nullptr;

  // Special case string object to use the standard text clipboard type.
  if( type == typeof(StringObject)) {
    cf = CF_TEXT;
    if (!IsClipboardFormatAvailable(cf)) {
      CloseClipboard();
      return nullptr; 
    }
  } else {
    while(0!=(cf=EnumClipboardFormats(cf))) {
      char name[256];
      GetClipboardFormatName(cf,name, 256);
      Type* cftype = Type::FindType(name);
      if(cftype && cftype->IsCastableTo(type)) {
        type = cftype;
        break;
      }
    }
    if(cf==0) {
      CloseClipboard();
      return nullptr;;
    }
  }

  Object* o = nullptr;

  HGLOBAL hg=GetClipboardData(cf);
  if(hg!=NULL) {
    if(cf == CF_TEXT) {
      String s((char*)GlobalLock(hg), 0, GlobalSize(hg));
      o = new StringObject(s);
      GlobalUnlock(hg);
    } else {
      // Create the resulting object
      o = type->CreateInstance();
      if(nullptr != o) {
        MemoryStream ms((noz_byte*)GlobalLock(hg), GlobalSize(hg));    
        BinaryDeserializer().Deserialize(&ms, o);
        GlobalUnlock(hg);
      }
    }      
  }

  CloseClipboard();

  return o;
}

bool Application::IsClipboardTypeAvailable (Type* type) {
  // Special case string object to use the standard text clipboard type.
  UINT cf;
  if(type == typeof(StringObject)) {
    cf = CF_TEXT;
  } else {
    cf = ::RegisterClipboardFormat(type->GetQualifiedName().ToCString());
  }

  return !!IsClipboardFormatAvailable(cf);
}

HRESULT WindowsWindow::OnDragEnter (Window* window, IDataObject* data, DWORD key_state, POINTL ptl, DWORD* effect) {
  Object* object = WindowsDragDrop::DataObjectToObject(data);
  if(nullptr == object) {
  	*effect = DROPEFFECT_NONE;
    return S_OK;
  }

  WindowsWindow* _this = (WindowsWindow*)window->GetHandle();

  POINT pt = {ptl.x, ptl.y};
  ScreenToClient(_this->cached_->hwnd,&pt);

  // Clear the cursor 
  window->SetCursor(nullptr);

  // Cache the drag drop object..
  _this->drag_drop_object_ = object;

  noz_uint32 modifiers = 0;
  if(key_state & MK_ALT) modifiers |= Keys::Alt;
  if(key_state & MK_CONTROL) modifiers |= Keys::Control;
  if(key_state & MK_SHIFT) modifiers |= Keys::Shift;

  // Issue a DragEnter system event to the window.
  SystemEvent::PushEvent(SystemEventType::DragEnter, window, object, Vector2((noz_float)pt.x, (noz_float)pt.y), modifiers);
  Application::Frame();

  switch(DragDrop::GetCurrentEffects()) {
    default:
    case DragDropEffects::None: *effect = DROPEFFECT_NONE; break;
    case DragDropEffects::Move: *effect = DROPEFFECT_MOVE; break;
    case DragDropEffects::Copy: *effect = DROPEFFECT_COPY; break;
    case DragDropEffects::Link: *effect = DROPEFFECT_LINK; break;
  }

	return S_OK;  
}

HRESULT WindowsWindow::OnDragOver (Window* window, DWORD key_state, POINTL ptl, DWORD* effect) {
  WindowsWindow* _this = (WindowsWindow*)window->GetHandle();
  if(nullptr == _this->drag_drop_object_) {
  	*effect = DROPEFFECT_NONE;
	  return S_OK;  
  }

  POINT pt = {ptl.x, ptl.y};
  ScreenToClient(_this->cached_->hwnd,&pt);

  noz_uint32 modifiers = 0;
  if(key_state & MK_ALT) modifiers |= Keys::Alt;
  if(key_state & MK_CONTROL) modifiers |= Keys::Control;
  if(key_state & MK_SHIFT) modifiers |= Keys::Shift;

  // Issue a DragOver system event to the window.
  SystemEvent::PushEvent(SystemEventType::DragOver, window, _this->drag_drop_object_, Vector2((noz_float)pt.x, (noz_float)pt.y), modifiers);
  Application::Frame();

  switch(DragDrop::GetCurrentEffects()) {
    default:
    case DragDropEffects::None: *effect = DROPEFFECT_NONE; break;
    case DragDropEffects::Move: *effect = DROPEFFECT_MOVE; break;
    case DragDropEffects::Copy: *effect = DROPEFFECT_COPY; break;
    case DragDropEffects::Link: *effect = DROPEFFECT_LINK; break;
  }

	return S_OK;  
}

HRESULT WindowsWindow::OnDragLeave (Window* window) {
  // Clear the drag drop object
  ((WindowsWindow*)window->GetHandle())->drag_drop_object_ = nullptr;

  // Issue a DragLeave system event to the window.
  SystemEvent::PushEvent(SystemEventType::DragLeave, window);
  Application::Frame();

	return S_OK;  
}

HRESULT WindowsWindow::OnDrop (Window* window, IDataObject* data_object, DWORD key_state, POINTL ptl, DWORD* effect) {
  WindowsWindow* _this = (WindowsWindow*)window->GetHandle();

  POINT pt = {ptl.x, ptl.y};
  ScreenToClient(_this->cached_->hwnd,&pt);

  noz_uint32 modifiers = 0;
  if(key_state & MK_ALT) modifiers |= Keys::Alt;
  if(key_state & MK_CONTROL) modifiers |= Keys::Control;
  if(key_state & MK_SHIFT) modifiers |= Keys::Shift;

  // Issue a DragLeave system event to the window.
  SystemEvent::PushEvent(SystemEventType::Drop, window, _this->drag_drop_object_, Vector2((noz_float)pt.x,(noz_float)pt.y), modifiers);
  Application::Frame();

  _this->drag_drop_object_ = nullptr;

	return S_OK;  
}

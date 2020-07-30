///////////////////////////////////////////////////////////////////////////////
// noZ Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_Windows_WindowsWindow_h__
#define __noz_Platform_Windows_WindowsWindow_h__

#include "WindowsDragDrop.h"
#include <noz/Platform/OpenGL/OpenGL.h>
#include <noz/Platform/WindowHandle.h>

namespace noz {
namespace Platform {
namespace Windows {

  struct CachedWindow {
    public: HWND hwnd;
    public: HDC hdc;
    public: HGLRC hglrc;
  };

  class WindowsWindow : public WindowHandle {
    public: static HINSTANCE hinstance_;
    public: Window* window_;
    public: bool is_popup_;
    public: bool is_tracking_;
    public: WindowsDragDrop::DropTarget* drop_target_;

    public: CachedWindow* cached_;

    public: static WindowsWindow* primary_;    
    public: static std::vector<CachedWindow*> cache_;

    public: HCURSOR cursor_;

    public: bool drag_drop_active_;
    public: ObjectPtr<Object> drag_drop_object_;

    public: WindowsWindow(Window* window, WindowAttributes attr);

    public: ~WindowsWindow(void);

    public: virtual void Show(void) override;

    public: virtual void SetTitle(String title) override;

    public: virtual Rect GetScreenRect(void) const override;

    public: virtual Rect GetClientRect(void) const override;

    public: virtual Rect ClientToScreen(const Rect& r) const override;

    public: Window* GetWindow(void) const {return window_;}

    public: virtual void BeginPaint(void) override;
    
    public: virtual void EndPaint(void) override;

    public: virtual void SetSize(noz_float width, noz_float height) override;
    
    public: virtual void Move(noz_float x, noz_float y) override;

    public: virtual void SetCapture(void) override;

    public: virtual void SetFocus (void) override;

    public: virtual void ReleaseCapture(void) override;

    public: virtual void SetCursor(Cursor* cursor) override;

    public: virtual void SetInteractive (bool interactive) override;

    public: virtual void Close (void) override;

    public: noz_uint32 GetKeyData(WPARAM wparam);

    public: noz_uint32 GetKeyData(char wparam);

    public: noz_uint32 GetModifiers (WPARAM wparam);

    public: Vector2 GetPosition(LPARAM lparam) {return Vector2((noz_float)GET_X_LPARAM(lparam), (noz_float)GET_Y_LPARAM(lparam));}

    public: static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);    

    protected: virtual Vector2 LocalToScreen (const Vector2& v) const override;
    
    protected: virtual Vector2 GetScreenSize (void) const override;

    private: void InitOpenGL(WindowsWindow* parent);

    private: static HRESULT OnDragEnter (Window* window, IDataObject* data_object, DWORD key_state, POINTL pt, DWORD* effect);
    private: static HRESULT OnDragOver (Window* window, DWORD key_state, POINTL pt, DWORD* effect);
    private: static HRESULT OnDragLeave (Window* window);
    private: static HRESULT OnDrop (Window* window, IDataObject* data_object, DWORD key_state, POINTL pt, DWORD* effect);
  };

} // namespace Windows
} // namespace Platform
} // namespace noz

#endif // __noz_Platform_Windows_WindowsWindow_h__


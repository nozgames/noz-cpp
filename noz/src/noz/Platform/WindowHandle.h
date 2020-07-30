///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_WindowHandle_h__
#define __noz_Platform_WindowHandle_h__

namespace noz { class Window; struct WindowAttributes; class Cursor; }

namespace noz {
namespace Platform {

  class WindowHandle {
    public: static WindowHandle* New(Window* window, WindowAttributes attr);

    public: virtual ~WindowHandle(void) {}

    /// Show the window.
    public: virtual void Show (void) = 0;

    /// Set the window title.
    public: virtual void SetTitle (String title) = 0;

    /// Return the rectangle that represents the window position and size on screen
    public: virtual Rect GetScreenRect(void) const = 0;

    /// Return the rectangle that represents the usable rectangle within the window
    public: virtual Rect GetClientRect(void) const = 0;

    /// Convert a client rectangle to a screen rectangle.
    public: virtual Rect ClientToScreen(const Rect& r) const = 0;

    public: virtual void BeginPaint(void) = 0;

    public: virtual void EndPaint(void) = 0;

    public: virtual void SetSize(noz_float width, noz_float height) = 0;
    
    public: virtual void SetCursor(Cursor* cursor) {}

    public: virtual void Move(noz_float x, noz_float y) = 0;

    public: virtual void SetCapture(void) = 0;

    public: virtual void SetFocus (void) = 0;

    public: virtual void ReleaseCapture(void) = 0;

    public: virtual Vector2 GetScreenSize (void) const = 0;

    public: virtual Vector2 LocalToScreen (const Vector2& v) const = 0;

    public: virtual void SetInteractive (bool interactive) { }

    public: virtual void Close (void) {}

    public: virtual void ShowKeyboard (bool show) {}
  };        


} // namespace Component
} // namespace noz


#endif // __noz_Components_WindowHandle_h__


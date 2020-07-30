///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Window_h__
#define __noz_Window_h__

#include <noz/Platform/WindowHandle.h>
#include <noz/UI/Cursors.h>
#include <noz/Node.h>
#include <noz/Nodes/UI/WindowNode.h>

namespace noz {

  class Component;
  class Popup;
  class WindowTransform;
  class WindowComponent;

  struct WindowAttributes {
    noz_uint32 value;
    WindowAttributes(void) {}
    WindowAttributes(noz_uint32 _value) {value=_value;}
    operator noz_uint32 (void) const {return value;}

    /// Window is the primary window in the system (when closed the appliation closes)
    static const noz_uint32 Primary = NOZ_BIT(0);

    /// Window is a popup window
    static const noz_uint32 Popup = NOZ_BIT(1);

    static const noz_uint32 Default = 0;
  };


  class Window : public Object {
    NOZ_OBJECT()

    friend class WindowNode;
    friend class Application;
    friend class Popup;

    /// Platform specific window handle..
    private: Platform::WindowHandle* handle_;

    /// Parent window.
    private: Window* parent_;

    /// Attributes window was created with
    private: WindowAttributes attr_;

    /// Graphics context for window.
    private: RenderContext* rc_;

    /// Cached client rectangle of the window
    private: Rect client_rect_;

    /// Cached screen rectangle of the window
    private: Rect screen_rect_;

    /// Map of mouse over node to frame number the node was determined to 
    /// have the mouse over it.
    private: std::map<Node*,noz_uint64> mouse_over_;

    /// System cursor
    private: ObjectPtr<Cursor> cursor_;

    private: ObjectPtr<Popup> popup_;

    private: WindowNode* root_node_;

    /// Node within the window that has focus
    private: ObjectPtr<UINode> focus_node_;

    private: bool closed_;

    private: bool interactive_;

    private: static Window* primary_window_;


    /// Default constructor
    public: Window (WindowAttributes attr=WindowAttributes::Default, Window* parent=nullptr);

    /// Default destructor
    public: ~Window(void);

    public: Platform::WindowHandle* GetHandle(void) const {return handle_;}


    public: bool HasFocus (void) const;

    public: void Close (void);

    public: void Show(void);

    public: void SetInteractive (bool interactive);

    public: void SetSize(const Vector2& size);

    public: bool IsPrimary (void) const {return !!(attr_ & WindowAttributes::Primary);}

    public: Window* GetParent (void) const {return parent_;}

    public: Node* GetRootNode (void) const {return root_node_;}
    
    public: Node* GetFocus (void) const {return focus_node_;}

    public: const Rect& GetScreenRect (void) const {return screen_rect_;}

    public: const Rect& GetClientRect (void) const {return client_rect_;}

    public: void DispatchEvent(SystemEvent* e);

    public: void ReleaseCapture (void);

    public: void SetCapture (void);

    public: void SetCursor (Cursor* cursor);

    /// Set the focus to the window and the node that was previously focused
    public: void SetFocus (void);

    /// Set the focus node within the window to the given node.
    public: void SetFocus (UINode* node);

    public: void SetTitle(const String& title) { handle_->SetTitle(title); }

    public: bool IsDescendantOf (Window* ancestor) const;

    public: void MoveTo (const Rect& r, Window* relative_to);

    public: bool IsPopup (void) const {return popup_ != nullptr;}

    public: bool IsClosed (void) const {return closed_;}

    public: Popup* GetPopup (void) const {return popup_;}

    public: Vector2 GetScreenSize (void) const;

    public: Vector2 LocalToScreen (const Vector2& v);

    public: void OnGainFocus (void);

    public: void OnLoseFocus (void);

    public: void OnSizeChanged  (void);

    public: void OnPositionChanged (void);

    public: static Window* GetPrimaryWindow (void) {return primary_window_;}

    private: void HandleKeyDown (SystemEvent* e);
    private: void HandlePreviewKeyDown (Node* n, SystemEvent* e);
  };        

} // namespace noz


#endif // __noz_Window_h__



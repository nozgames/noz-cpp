///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Popup.h>
#include "Window.h"

using namespace noz;
using namespace noz::Platform;

Window* Window::primary_window_ = nullptr;

Window::Window(WindowAttributes attr, Window* parent) {
  closed_ = false;
  interactive_= true;

  // Parent window
  parent_ = parent;

  // Create the window handle..
  handle_ = nullptr;
  handle_ = WindowHandle::New(this,attr);

  // Save attributes
  attr_ = attr;

  // Create a context for rendering..
  rc_ = new RenderContext;

  // Get window size..
  screen_rect_ = handle_->GetScreenRect();
  client_rect_ = handle_->GetClientRect();

  cursor_ = Cursors::Default;

  // Parent ourselves to the root..  
  if(!Application::IsInitializing()) {
    root_node_ = new WindowNode(this);  
    root_node_->SetParent(Application::GetRootNode());
  } else {
    root_node_ = nullptr;
  }

  if(attr & WindowAttributes::Primary) primary_window_ = this;
}

Window::~Window(void) {
  SetFocus(nullptr);

  if(popup_) popup_->window_ = nullptr;

  if(root_node_) {
    root_node_->Orphan();
    root_node_->Destroy();
  }

  delete handle_;
}

void Window::Close (void) {
  closed_ = true;
}

void Window::Show(void) {
  handle_->Show();
}

void Window::SetSize(const Vector2& size) {
  handle_->SetSize(size.x, size.y);
}

void Window::HandlePreviewKeyDown (Node* n, SystemEvent* e) {
  if(nullptr==n) return;

  HandlePreviewKeyDown(n->GetParent(),e);

  if(e->IsHandled()) return;

  n->OnPreviewKeyDown(e);
}

void Window::HandleKeyDown (SystemEvent* e) {
  if(focus_node_==nullptr) return;

  HandlePreviewKeyDown(focus_node_->GetParent(),e);

  for(Node* n=focus_node_; !e->IsHandled() && n; n=n->GetParent()) {
    n->OnKeyDown(e);
  }

  if(!e->IsHandled()) {
    NOZ_TODO("Focus traversal");
    /*
    // If the event was not handled and it was a Tab key press then attempt to shift focus.
    if(!ke->IsHandled() && ke->GetKeyState() == KeyEvent::KeyState::Down && ke->GetKeyCode() == Keys::Tab) {        
      if(ke->IsShift()) {          
        for(Node* node=focus_->GetPrev(NodeTreeType::Visual,nullptr,true); focus_!=node && node ; node=node->GetPrev(NodeTreeType::Visual,nullptr,true)) {
          if(node->IsFocusable()) {SetFocus(node); break;}
        }
      } else {
        for(Node* node=focus_->GetNext(NodeTreeType::Visual,nullptr,true); focus_!=node && node; node=node->GetNext(NodeTreeType::Visual,nullptr,true)) {
          if(node->IsFocusable()) {SetFocus(node); break;;}
        }
      }
    }*/
  }
}

void Window::DispatchEvent(SystemEvent* e) {
  switch(e->GetEventType()) {
    case SystemEventType::KeyDown: HandleKeyDown(e); break;

    case SystemEventType::KeyUp: {
      if(focus_node_) focus_node_->OnKeyUp(e);
      break;
    }
    
    default: break;
  } 
}

void Window::SetCursor (Cursor* cursor) {
  if(cursor_ == cursor) return;

  if(cursor == nullptr) {
    cursor_ = Cursors::Default;
  } else {
    cursor_ = cursor;
  }

  handle_->SetCursor(cursor_);
}


void Window::MoveTo (const Rect& r, Window* relative_to) {
  if(relative_to) {
    Rect srect = relative_to->handle_->ClientToScreen(r);
    handle_->Move(srect.x,srect.y);
    handle_->SetSize(srect.w,srect.h);
  } else {
    handle_->Move(r.x,r.y);
    handle_->SetSize(r.w,r.h);
  }    
}

Vector2 Window::LocalToScreen (const Vector2& v) {
  if(handle_) return handle_->LocalToScreen(v);
  return v;
}

bool Window::HasFocus(void) const {
  return Application::GetFocus() == this;
}

Vector2 Window::GetScreenSize (void) const {
  noz_assert(handle_);
  return handle_->GetScreenSize();
}

void Window::SetFocus(void) { 
  if(HasFocus()) return;
  handle_->SetFocus();
  Application::SetFocus(this);
}

void Window::SetFocus(UINode* focus) {
  // Ensure the node belongs to this window
  if(focus && focus->GetWindow() != this) return;

  UINode* old_focus = focus_node_;
  focus_node_ = focus;

  if(old_focus) old_focus->OnLoseFocus();

  if(HasFocus()) {
    if(focus) focus->OnGainFocus();

    // Fire focus changed event.
    Application::FocusChanged(this, focus);

  } else {
    SetFocus();
  }

  handle_->ShowKeyboard (focus && focus->IsKeyboardRquired());
}

void Window::SetCapture(void) {
  noz_assert(handle_);
  handle_->SetCapture();
}

void Window::ReleaseCapture(void) {
  noz_assert(handle_);
  handle_->ReleaseCapture();
}

bool Window::IsDescendantOf (Window* ancestor) const {
  if(ancestor==nullptr) return false;
  for(Window* w = parent_; w; w=w->parent_) if(w==ancestor) return true;
  return false;
}

void Window::OnGainFocus (void) {
  if(focus_node_) focus_node_->OnGainFocus();

  // Fire focus changed event.
  Application::FocusChanged(this, focus_node_);
}

void Window::OnLoseFocus (void) {
  if(focus_node_) focus_node_->OnLoseFocus();
}

void Window::SetInteractive(bool interactive) {
  if(interactive == interactive_) return;
  interactive_ = interactive;
  handle_->SetInteractive(interactive_);
}

void Window::OnSizeChanged  (void) {
  if(nullptr==handle_) return;
  screen_rect_ = handle_->GetScreenRect();
  client_rect_ = handle_->GetClientRect();
  root_node_->InvalidateTransform();  
}

void Window::OnPositionChanged (void) {
  if(nullptr==handle_) return;
  screen_rect_ = handle_->GetScreenRect();
}


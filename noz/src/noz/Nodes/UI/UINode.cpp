///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "UINode.h"

using namespace noz;

UINode::UINode (void) {
  interactive_ = true;
  inherited_interactive_ = true;
  tab_navigation_ = TabNavigation::Continue;
  focused_ = false;
  focusable_ = false;
  drag_drop_target_ = false;
  drag_drop_source_ = false;
  requires_keyboard_ = false;
  NOZ_FIXME()
  //EnableEvent(ComponentEvent::Mouse);
}

void UINode::SetDragDropTarget (bool v) {
  drag_drop_target_ = v;
}

void UINode::SetDragDropSource (bool v) {
  drag_drop_source_ = v;
}

void UINode::SetRequiresKeyboard (bool v) {
  requires_keyboard_ = v;
}

void UINode::SetInteractive(bool interactive) {
  // Ensure the state changed
  if(interactive_ == interactive) return;

  // Set new local interactive state
  interactive_ = interactive;

  OnInteractiveChanged();

  // Propegate the change to children.  
  for(noz_uint32 i=0,c=GetPrivateChildCount(); i<c; i++) {
    PropagateInteractiveChange(GetPrivateChild(i), IsInteractive());
  }
  for(noz_uint32 i=0,c=GetChildCount(); i<c; i++) {
    PropagateInteractiveChange(GetChild(i), IsInteractive());
  }
}

void UINode::OnLineageChanged (void) {
  // If nested within another control inherit its inactive state.
  bool new_inherited_interactive = true;
 
  // Retrieve the closest ancestor node of UINode type
  UINode* ancestor = GetAncestor<UINode>();
  if(ancestor) {
    new_inherited_interactive = ancestor->interactive_;
  }

  // IF the inherited interactive state did not change then we are done.
  if(new_inherited_interactive == inherited_interactive_) return;
    
  // Update the interactive state of the node recursively
  PropagateInteractiveChange(this, new_inherited_interactive);
}

void UINode::PropagateInteractiveChange (Node* node, bool inherited) {
  if(node->IsTypeOf(typeof(UINode))) {
    UINode* uinode = (UINode*)node;

    // Make sure the inherited value is actually changing
    if(uinode->inherited_interactive_ == inherited) return;

    // Update inherited value
    uinode->inherited_interactive_ = inherited;

    // Inform node that its interactive state changed
    uinode->OnInteractiveChanged ();

    inherited = uinode->IsInteractive();
  }  

  // Tell our children.
  for(noz_uint32 i=0,c=node->GetPrivateChildCount(); i<c; i++) {
    PropagateInteractiveChange(node->GetPrivateChild(i), inherited);
  }
  for(noz_uint32 i=0,c=node->GetChildCount(); i<c; i++) {
    PropagateInteractiveChange(node->GetChild(i), inherited);
  }
}

void UINode::OnInteractiveChanged (void) {
  NOZ_FIXME()
  //EnableEvent(ComponentEvent::Mouse,IsInteractive());
}

void UINode::SetCursor(Cursor* cursor) {
  cursor_ = cursor;
}

void UINode::SetTabNavigation (TabNavigation tab) {
  if(tab_navigation_ == tab) return;
  tab_navigation_ = tab;
}

UINode* UINode::GetNextFocusable (void) {
NOZ_FIXME()
#if 0
  ComponentManager::Update();

  // Find a parent with a non "continue" tab navigation type
  UINode* tab_parent = nullptr;
  for(Node* p=GetNode(); !tab_parent && p; p=p->GetParent()) {
    Component* pc = p->GetParentComponent();
    if(nullptr == pc || !pc->IsTypeOf(typeof(UINode))) continue;
    if(((UINode*)pc)->tab_navigation_==TabNavigation::Continue) continue;
    tab_parent = (UINode*)pc;
  }

  // Loop forward until we find a candidate
  for(Component* next=ComponentManager::GetNextComponentWithinParent(this,tab_parent); 
      next;
      next=ComponentManager::GetNextComponentWithinParent(next,tab_parent)) {
    if(!next->GetNode()->IsVisible()) continue;
    if(!next->IsTypeOf(typeof(UINode))) continue;
    if(!((UINode*)next)->focusable_) continue;    
    if(!((UINode*)next)->IsInteractive()) continue;    
    return (UINode*)next;
  }

  NOZ_TODO("cycle");
#endif
  return nullptr;
}

void UINode::OnLoseFocus(void) {
  focused_ = false;
  LostFocus(this);
}

void UINode::OnGainFocus(void) {
  focused_= true;
  GainFocus(this);
}

void UINode::SetFocusable (bool v) {
  if(v==focusable_) return;

  // Set the value
  focusable_ = v; 

  // If no longer focusable and have focus, find an appropriate replacement for focus
  if(!focusable_ && HasFocus()) {
    Window* w = GetWindow();
    if(w) w->SetFocus(nullptr);
  }
}

void UINode::SetFocus (void) {
  Window* w = GetWindow();
  if(w) w->SetFocus(this);
}

void UINode::OnDragDrop (DragDropEventArgs* args) {
  DragDrop(this, args);
}

void UINode::OnMouseOver (SystemEvent* e) {
  Node::OnMouseOver(e);
  if(cursor_) e->SetCursor(cursor_);

  if(drag_drop_source_ && HasCapture()) {
    if(Input::GetMouseButtonDrag(MouseButton::Left)) DoDragDrop();
  }
}

void UINode::OnMouseDown (SystemEvent* e) {
  Node::OnMouseDown(e);

  if(drag_drop_source_) {
    if(e->GetButton()==MouseButton::Left &&
       e->GetClickCount() == 1) {
      SetCapture();
    }
  }
}

void UINode::OnMouseUp (SystemEvent* e) {
  Node::OnMouseUp(e);

  if(drag_drop_source_) {
    if(e->GetButton()==MouseButton::Left && HasCapture()) {
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/ScrollView.h>
#include "TreeView.h"
#include "TreeViewItem.h"

using namespace noz;

std::vector<ObjectPtr<TreeViewItem>> TreeView::saved_selected_items_;

TreeView::TreeView (void) {
  selection_mode_ = SelectionMode::Single;
  SetLogicalChildrenOnly();
  if(!Application::IsInitializing()) {
    Application::FocusChanged += FocusChangedEventHandler::Delegate(this, &TreeView::OnFocusChanged);
  }
}

TreeView::~TreeView(void) {  
}

bool TreeView::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == items_container_) return false;

  // Reparent all items
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    items_container_->AddChild(GetLogicalChild(i));
  }

  return true;
}

void TreeView::OnStyleChanged (void) {
  Control::OnStyleChanged();

  // Orphan our logical children from their visual parent
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) GetLogicalChild(i)->Orphan(false);

  items_container_ = nullptr;
}

void TreeView::UnselectAll (void) {
  // Ensure there are actually items selected
  if(selected_items_.empty()) return;

  UnselectAllNoEvent();
  SelectionChanged(this);
}

void TreeView::UnselectAllNoEvent (void) {
  for(auto it=selected_items_.begin(); it!=selected_items_.end(); it=selected_items_.begin()) {
    (*it)->SetSelectedInternal(false,true);
  }
}

void TreeView::AdjustFocusForRemove (noz_uint32 index) {
  NOZ_TODO("implement");
}

void TreeView::SetFocusedItem (TreeViewItem* tvitem) {
  if(focus_item_==tvitem) return;

  TreeViewItem* old = focus_item_;
  focus_item_ = tvitem;

  if(old) old->UpdateAnimationState();
  if(focus_item_) focus_item_->UpdateAnimationState();

  focus_item_ = tvitem;
}

TreeViewItem* TreeView::GetFirstChildItem (void) const {
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    Node* child = GetLogicalChild(i);
    if(child->IsTypeOf(typeof(TreeViewItem))) return (TreeViewItem*)child;
  }
  return nullptr;
}

TreeViewItem* TreeView::GetLastChildItem (void) const {
  for(noz_uint32 i=GetLogicalChildCount(); i>0; i--) {
    Node* child = GetLogicalChild(i-1);
    if(child->IsTypeOf(typeof(TreeViewItem))) return (TreeViewItem*)child;
  }
  return nullptr;
}

void TreeView::OnGainFocus (void) {
  Control::OnGainFocus();
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    Node* child = GetLogicalChild(i);
    if(child->IsTypeOf(typeof(TreeViewItem))) ((TreeViewItem*)child)->UpdateAnimationState();
  }
}

void TreeView::OnLoseFocus (void) {
  Control::OnLoseFocus();
  for(TreeViewItem* tvitem=GetFirstChildItem(); tvitem; tvitem=tvitem->GetNextVisibleItem()) {
    tvitem->UpdateAnimationState();
  }
}

void TreeView::OnFocusChanged(Window* window, UINode* new_focus) {
  // If the new focus is a child of the tab control..
  if(new_focus && (new_focus==this || new_focus->IsChildOf(this))) {
    SetAnimationState(UI::StateFocused);
  } else {
    SetAnimationState(UI::StateUnFocused);
  }

  for(TreeViewItem* tvitem=GetFirstChildItem(); tvitem; tvitem=tvitem->GetNextVisibleItem()) {
    tvitem->UpdateAnimationState();
  }
}

void TreeView::SetSelectedItem (TreeViewItem* item) {
  if(item==nullptr) {
    UnselectAll();
    return;
  }

  if(item->tree_view_ != this) return;

  UnselectAllNoEvent();
  item->SetSelected(true);
}

void TreeView::Select (TreeViewItem* item) {
  noz_assert(item);
  if(item->tree_view_ != this) return;
  item->SetSelected(true);
}

void TreeView::Unselect (TreeViewItem* item) {
  noz_assert(item);
  if(item->tree_view_ != this) return;
  item->SetSelected(false);
}

void TreeView::ExpandAll (void) {
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    Node* child = GetLogicalChild(i);
    if(child->IsTypeOf(typeof(TreeViewItem))) ((TreeViewItem*)child)->ExpandAll();
  }
}

void TreeView::OnChildAdded (Node* child) {
  Control::OnChildAdded(child);

  if(!child->IsPrivate() && child->IsTypeOf(typeof(TreeViewItem))) {
    ((TreeViewItem*)child)->SetTreeView(this,nullptr);
  }

  if(items_container_) items_container_->AddChild(child);
}


void TreeView::OnChildRemoved (Node* child) {
  Control::OnChildRemoved(child);

  if(!child->IsPrivate() && child->IsTypeOf(typeof(TreeViewItem))) {
    ((TreeViewItem*)child)->SetTreeView(nullptr,nullptr);
  }  
}

void TreeView::OnMouseWheel (SystemEvent* e) {
  Control::OnMouseWheel(e);

  if(scroll_view_==nullptr) return;
  if(!HasLogicalChildren()) return;

  scroll_view_->SetVerticalOffset(
    scroll_view_->GetVerticalOffset() + -e->GetDelta().y * 16);
}

void TreeView::OnKeyDown (SystemEvent* e) {
#if defined(NOZ_WINDOWS)  
  // On windows F2 should trigger in place editing.
  if(e->GetKeyCode() == Keys::F2 && e->GetModifiers()==0) {
    if(selected_items_.size()==1) GetSelectedItem()->BeginTextEdit();
    e->SetHandled();
    return;
  }
#endif

  // Navigate down to next visible item
  if(e->GetKeyCode() == Keys::Down) {
    e->SetHandled();

    if(nullptr == focus_item_) {
      if(GetFirstChildItem()) SetFocusedItem(GetFirstChildItem());
      if(nullptr == focus_item_) return;
    }
      
    TreeViewItem* next_item = focus_item_->GetNextVisibleItem();
    if(nullptr == next_item) return;
    if(e->IsControl()) {
      SetFocusedItem(next_item);
      selection_anchor_ = next_item;
    } else if(next_item->HandleSelection(e->GetModifiers())) {
      SelectionChanged(this);
    }
    return;
  }

  // Navigate up to next visible item
  if(e->GetKeyCode() == Keys::Up) {
    e->SetHandled();
    if(nullptr == focus_item_) {
      if(GetFirstChildItem()) SetFocusedItem(GetFirstChildItem());
      if(nullptr == focus_item_) return;
    }
      
    TreeViewItem* prev_item = focus_item_->GetPrevVisibleItem();
    if(nullptr == prev_item) return;
    if(e->IsControl()) {
      SetFocusedItem(prev_item);
      selection_anchor_ = prev_item;
    } else if(prev_item->HandleSelection(e->GetModifiers())) {
      SelectionChanged(this);
    }
    return;
  }

  // Toggle selection at focus item
  if(e->GetKeyCode() == Keys::Space) {
    e->SetHandled();
    if(focus_item_) if(focus_item_->HandleSelection(Keys::Control)) SelectionChanged(this);
    return;
  }

  // Expand/Contract focus item
  if(e->GetKeyCode() == Keys::Return) {
    e->SetHandled();
    if(focus_item_ && focus_item_->IsSelected()) focus_item_->SetExpanded(!focus_item_->IsExpanded());
    return;
  }
  
  Control::OnKeyDown(e);
}

void TreeView::Update (void) {
  Control::Update();

  // Handle text editing.
  if(nullptr == text_edit_pending_) return;

  // If the mouse is being dragged cancel the edit
  if(Input::GetMouseButtonDrag(MouseButton::Left)) {
    text_edit_pending_ = nullptr;
    return;
  }

  if(Time::GetTime() - Input::GetMouseButtonTime(MouseButton::Left) < 0.5f) return;

  text_edit_pending_->BeginTextEdit(false);
}

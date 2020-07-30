///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ListView.h"
#include "ListViewItem.h"
#include "ScrollView.h"

using namespace noz;


ListView::ListView (void) {
  focus_item_ = nullptr;
  selection_mode_ = SelectionMode::Single;
  SetLogicalChildrenOnly();

  if(!Application::IsInitializing()) {
    Application::FocusChanged += FocusChangedEventHandler::Delegate(this, &ListView::OnFocusChanged);
  }
}

ListView::~ListView (void) {
}

bool ListView::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == items_container_) return false;

  // Reparent all items
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) items_container_->AddChild(GetLogicalChild(i));

  return true;
}

void ListView::OnStyleChanged (void) {
  Control::OnStyleChanged();

  // Orphan our logical children from their visual parent
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) GetLogicalChild(i)->Orphan(false);

  items_container_ = nullptr;
}

void ListView::UnselectAll (void) {
  if(selected_items_.empty()) return;

  UnselectAllNoEvent();
  SelectionChanged(this);
}

void ListView::UnselectAllNoEvent (void) {
  for(auto it=selected_items_.begin(); it!=selected_items_.end(); it=selected_items_.begin()) {
    (*it)->SetSelected(false);
  }
}

void ListView::SetFocusedItem(ListViewItem* lvitem, bool anchor) {
  if(focus_item_==lvitem) return;

  ListViewItem* old = focus_item_;
  focus_item_ = lvitem;

  if(old) old->UpdateAnimationState();
  if(focus_item_) focus_item_->UpdateAnimationState();

  if(anchor) selection_anchor_ = lvitem;
}

ListViewItem* ListView::GetFirstChildItem (void) const {
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    Node* child = GetLogicalChild(i);
    if(child->IsTypeOf(typeof(ListViewItem))) return (ListViewItem*)child;
  }
  return nullptr;
}

ListViewItem* ListView::GetLastChildItem (void) const {
  for(noz_uint32 i=GetLogicalChildCount(); i>0; i--) {
    Node* child = GetLogicalChild(i-1);
    if(child->IsTypeOf(typeof(ListViewItem))) return (ListViewItem*)child;
  }
  return nullptr;
}

void ListView::SortChildren (noz_int32 (*sort_proc) (const Node* node1, const Node* node2)) {
  if(items_container_) {
    for(noz_uint32 i=0,c=GetLogicalChildCount();i<c;i++) GetLogicalChild(i)->Orphan(false);
  }

  Control::SortChildren(sort_proc);

  if(items_container_) {
    for(noz_uint32 i=0,c=GetLogicalChildCount();i<c;i++) items_container_->AddChild(GetLogicalChild(i));
  }
}

void ListView::OnFocusChanged(Window* window, UINode* new_focus) {
  for(noz_uint32 i=0,c=GetLogicalChildCount();i<c;i++) {
    Node* child = GetLogicalChild(i);
    noz_assert(child);
    if(child->IsTypeOf(typeof(ListViewItem))) ((ListViewItem*)child)->UpdateAnimationState();
  }
}

void ListView::OnMouseDown (SystemEvent* e) {
  Control::OnMouseDown(e);

  // Unselect all items if a click in the list view does not hit anything
  UnselectAll();

  e->SetHandled();
  SetFocus();
}

void ListView::OnMouseWheel (SystemEvent* e) {
  Control::OnMouseWheel(e);

  if(scroll_view_==nullptr) return;
  if(!HasLogicalChildren()) return;

  scroll_view_->SetVerticalOffset(
    scroll_view_->GetVerticalOffset() + -e->GetDelta().y * GetLogicalChild(0)->GetMeasuredSize().y);
}

void ListView::OnChildAdded (Node* child) {
  Control::OnChildAdded(child);

  if(child->IsTypeOf(typeof(ListViewItem))) ((ListViewItem*)child)->SetListView(this);

  if(items_container_) items_container_->AddChild(child);
}

void ListView::OnChildRemoved (Node* child) {
  Control::OnChildRemoved (child);

  if(child->IsTypeOf(typeof(ListViewItem))) ((ListViewItem*)child)->SetListView(nullptr);
}

void ListView::OnKeyDown (SystemEvent* e) {
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
      
    ListViewItem* next_item = focus_item_->GetNextSiblingItem();
    if(nullptr == next_item) return;
    if(e->IsControl()) {
      SetFocusedItem(next_item,true);
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
      
    ListViewItem* prev_item = focus_item_->GetPrevSiblingItem();
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

  // Open
  if(e->GetKeyCode() == Keys::Return && e->GetModifiers() == 0) {
    e->SetHandled();
    if(focus_item_ && focus_item_->IsSelected()) focus_item_->OnOpen();
    return;
  }
 
  Control::OnKeyDown(e);
}

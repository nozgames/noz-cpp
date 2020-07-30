///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "DropDownList.h"
#include "DropDownListItem.h"
#include "ScrollView.h"

using namespace noz;


DropDownList::DropDownList (void) {
  SetLogicalChildrenOnly();
  selected_item_index_= -1;

  if(!Application::IsInitializing()) {
    Application::FocusChanged += FocusChangedEventHandler::Delegate(this, &DropDownList::OnFocusChanged);
  }
}

DropDownList::~DropDownList (void) {
}

void DropDownList::CommitSelection (void) {
  if(popup_) {
    if(popup_->IsOpen()) {
      popup_->Close();
    } else {
      popup_->Open();
    }
  }  

  UpdateSelectedItem();
}

bool DropDownList::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == items_container_) return false;

  // Reparent all items
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) items_container_->AddChild(GetLogicalChild(i));

  UpdateSelectedItem();

  return true;
}

void DropDownList::OnStyleChanged (void) {
  Control::OnStyleChanged();

  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) GetLogicalChild(i)->Orphan(false);

  items_container_ = nullptr;
}

void DropDownList::UnselectAllNoEvent (void) {
  if(selected_item_index_==-1) return;

  DropDownListItem* item = GetSelectedItem();
  noz_assert(item);

  // Remove the selection
  selected_item_index_ = -1;

  // Update the previously selected item
  item->UpdateAnimationState();
}

void DropDownList::UnselectAll(void) {
  if(selected_item_index_==-1) return;
  UnselectAllNoEvent();
  SelectionChanged(this);
}

void DropDownList::SortChildren(noz_int32(*sort_proc)(const Node *node1, const Node *node2)) {
  if(items_container_) {
    for(noz_uint32 i=0,c=GetLogicalChildCount();i<c;i++) GetLogicalChild(i)->Orphan(false);
  }

  Node::SortChildren(sort_proc);

  if(items_container_) {
    for(noz_uint32 i=0,c=GetLogicalChildCount();i<c;i++) items_container_->AddChild(GetLogicalChild(i));
  }
}

void DropDownList::OnFocusChanged(Window* window, UINode* new_focus) {
  for(noz_uint32 i=0,c=GetLogicalChildCount();i<c;i++) {
    Node* child = GetLogicalChild(i);
    noz_assert(child);
    if(child->IsTypeOf(typeof(DropDownListItem))) ((DropDownListItem*)child)->UpdateAnimationState();
  }
}

void DropDownList::OnMouseDown(SystemEvent* e) {    
  noz_assert(e);
  noz_assert(e->GetEventType() == SystemEventType::MouseDown);

  // Handle non interactive state.
  if(!IsInteractive()) return;

  // We only care about left ButtonBase presses
  if(e->GetButton() != MouseButton::Left) return;

  // Mark the event as handled so it does not propegate further
  e->SetHandled();

  // Set the button as focused
  if(IsFocusable()) SetFocus();

  // Set the capture to this node to ensure all future events are routed to it.
  SetCapture();

  // Update the state
  UpdateAnimationState();
}

void DropDownList::OnMouseUp(SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType() == SystemEventType::MouseUp);

  // Update state
  if(!HasCapture()) return;

  // Release capture and update animation state.
  ReleaseCapture();
  UpdateAnimationState();

  // If there are no items in the dropdown list then dont show the popup
  if(GetLogicalChildCount()==0) return;

  if(IsMouseOver() && popup_) {
    if(popup_->IsOpen()) {
      popup_->Close();
    } else {
      popup_->Open();
    }
  }
}

void DropDownList::OnMouseWheel (SystemEvent* e) {
  Control::OnMouseWheel(e);
/*
  if(scroll_view_==nullptr) return;
  if(items_.empty()) return;

  scroll_view_->SetVerticalOffset(scroll_view_->GetVerticalOffset() + -e->GetDelta().y * items_[0]->GetMeasuredSize().y);
*/
}

void DropDownList::UpdateAnimationState(void) {
  if(HasCapture()) {
    if(IsMouseOver()) {
      SetAnimationState(UI::StatePressed);
    } else {
      SetAnimationState(UI::StateMouseOver);
    }
    return;
  }

  Control::UpdateAnimationState();
}

void DropDownList::UpdateSelectedItem(void) {
  DropDownListItem * selected_item = GetSelectedItem();
  if(selected_item) {
    if(text_node_) text_node_->SetText(selected_item->GetText());
    if(sprite_node_) sprite_node_->SetSprite(selected_item->GetSprite());
  } else {
    if(text_node_) text_node_->SetText("");
    if(sprite_node_) sprite_node_->SetSprite(nullptr);
  }
}

void DropDownList::SetSelectedItemIndex (noz_uint32 index) {
  if(selected_item_index_ == index) return;
  UnselectAllNoEvent();
  selected_item_index_ = index;
  if(IsDeserializing()) return;
  if(GetLogicalChildCount()==0) {
    selected_item_index_ = -1;
  } else {
    selected_item_index_ = Math::Clamp(selected_item_index_,0,GetLogicalChildCount()-1);
    if(!GetLogicalChild(selected_item_index_)->IsTypeOf(typeof(DropDownListItem))) {
      selected_item_index_ = -1;
    }
  }
  if(selected_item_index_!=-1) {
    ((DropDownListItem*)GetLogicalChild(selected_item_index_))->UpdateAnimationState();
  }

  UpdateSelectedItem();
  SelectionChanged(this);
}

void DropDownList::OnDeserialized (void) {
  Control::OnDeserialized();

  if(selected_item_index_ != -1) {
    noz_uint32 index = selected_item_index_;
    selected_item_index_ = -1;
    SetSelectedItemIndex(index);
  }
}

DropDownListItem* DropDownList::GetFirstChildItem (void) const {
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    Node* child = GetLogicalChild(i);
    if(child->IsTypeOf(typeof(DropDownListItem))) return (DropDownListItem*)child;
  }
  return nullptr;
}

DropDownListItem* DropDownList::GetLastChildItem (void) const {
  for(noz_uint32 i=GetLogicalChildCount(); i>0; i--) {
    Node* child = GetLogicalChild(i-1);
    if(child->IsTypeOf(typeof(DropDownListItem))) return (DropDownListItem*)child;
  }
  return nullptr;
}

void DropDownList::OnChildAdded (Node* child) {
  Control::OnChildAdded(child);

  if(child->IsTypeOf(typeof(DropDownListItem))) ((DropDownListItem*)child)->drop_down_list_ = this;

  if(items_container_) items_container_->AddChild(child);    
}

void DropDownList::OnChildRemoved (Node* child) {
  Control::OnChildRemoved(child);

  if(child->IsTypeOf(typeof(DropDownListItem))) {
    ((DropDownListItem*)child)->drop_down_list_ = nullptr;
    noz_uint32 index = selected_item_index_;
    selected_item_index_ = -1;
    if(GetLogicalChildCount()>0) {
      SetSelectedItemIndex(Math::Min(GetLogicalChildCount()-1,index));
    } else {
      UpdateSelectedItem();
    }      
  }
}


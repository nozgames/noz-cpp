///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TabControl.h"
#include "TabItem.h"

using namespace noz;


TabControl::TabControl(void) {
  if(!Application::IsInitializing()) {
    Application::FocusChanged += FocusChangedEventHandler::Delegate(this, &TabControl::OnFocusChanged);
  }
  active_ = false;
  SetLogicalChildrenOnly();
}

TabControl::~TabControl(void) {
}

TabItem* TabControl::GetFirstChildItem(void) const {
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    Node* child = GetLogicalChild(i);
    if(child->IsTypeOf(typeof(TabItem))) return (TabItem*)child;
  }
  return nullptr;
}

TabItem* TabControl::GetLastChildItem(void) const {
  for(noz_uint32 i=GetLogicalChildCount(); i>0; i--) {
    Node* child = GetLogicalChild(i-1);
    if(child->IsTypeOf(typeof(TabItem))) return (TabItem*)child;
  }
  return nullptr;
}

bool TabControl::OnApplyStyle(void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == items_container_) return false;
  if(nullptr == content_container_) return false;

  // Reparent all items
  if(items_container_ && content_container_) {
    for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
      Node* child = GetLogicalChild(i);
      items_container_->AddChild(child);

      if(child->IsTypeOf(typeof(TabItem))) {
        TabItem* item = ((TabItem*)child);
        item->SetTabControl(this);
        content_container_->AddChild(item->content_container_);
        item->content_container_->SetVisibility(item->IsSelected()?Visibility::Visible:Visibility::Collapsed);
      }
    }
  }

  return true;
}

void TabControl::OnChildAdded(Node * child) {
  Control::OnChildAdded(child);

  if(child->IsTypeOf(typeof(TabItem))) {
    TabItem* item = (TabItem*)child;
    item->SetTabControl(this);
    if(selected_==nullptr) {
      item->Select();
    }
    UpdateAnimationState();
  }

  if(items_container_) items_container_->AddChild(child);
}

void TabControl::OnStyleChanged(void) {
  Control::OnStyleChanged();

  // Orphan our logical children from their visual parent
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    GetLogicalChild(i)->Orphan(false);
    if (GetLogicalChild(i)->IsTypeOf(typeof(TabItem))) {
      TabItem* item = (TabItem*)GetLogicalChild(i);
      item->content_container_->Orphan();
    }
  }

  items_container_ = nullptr;
  content_container_ = nullptr;
}

void TabControl::UpdateAnimationState(void) {
  Control::UpdateAnimationState();

  if(GetFirstChildItem()==nullptr) {
    SetAnimationState(UI::StateEmpty);
  } else {
    SetAnimationState(UI::StateNotEmpty);
  }

  if(active_) {
    SetAnimationState(UI::StateFocused);
  } else {
    SetAnimationState(UI::StateUnFocused);
  }  
}

void TabControl::OnFocusChanged(Window* window, UINode* new_focus) {
  // Dont care about the focus of another window
  if(window != GetWindow()) return;

  // If the new focus is a child of the tab control..
  bool active = active_;
  if(new_focus && (new_focus==this || new_focus->IsDescendantOf(this))) {
    active = true;
  } else {
    active = false;
  }

  if(active != active_) {
    active_ = active;
    UpdateAnimationState();
    if(selected_) selected_->UpdateAnimationState();
  }
}

void TabControl::OnGainFocus (void) {
  Control::OnGainFocus();
  for(TabItem* item=GetFirstChildItem(); item; item=item->GetNextSiblingItem()) item->UpdateAnimationState();
}

void TabControl::OnLoseFocus (void) {
  Control::OnLoseFocus();
  for(TabItem* item=GetFirstChildItem(); item; item=item->GetNextSiblingItem()) item->UpdateAnimationState();
}


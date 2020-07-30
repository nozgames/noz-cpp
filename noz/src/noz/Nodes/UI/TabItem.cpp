///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TabItem.h"
#include "TabControl.h"

using namespace noz;


TabItem::TabItem(void) {
  tc_ = nullptr;
  SetLogicalChildrenOnly();

  if(!Application::IsInitializing()) {
    content_container_ = new Node;
    content_container_->SetVisibility(Visibility::Collapsed);
  }
}

TabItem::~TabItem(void) {
  if(content_container_) content_container_->Destroy();
}

bool TabItem::IsSelected (void) const {
  return tc_ && tc_->selected_== this;
}

void TabItem::SetTabControl (TabControl* t) {
  // Ensure something actually changed.
  if(tc_ == t) return;

  // Set tab control pointer
  tc_ = t;

  if(tc_) {
    if(tc_->content_container_) tc_->content_container_->AddChild(content_container_);
  } else {
    content_container_->Orphan();
  }

  // Select the item if being parented to a tab control that is awake and has no selection
  if(tc_ && tc_->IsAwake() && tc_->selected_==nullptr) Select();
}

void TabItem::OnMouseDown (SystemEvent* e) {
  if(e->GetButton() != MouseButton::Left) return;
  if(e->GetClickCount() != 1) return;

  Select();

  e->SetHandled();
  if(tc_) tc_->SetFocus();
}

void TabItem::Select (void) {
  // Ensure there is an paired tabcontrol and that this tab item isnt already selected.
  if(tc_ == nullptr) return;
  if(tc_->selected_ == this) return;

  TabItem* old_selection = tc_->selected_;

  // Mark as selected
  tc_->selected_ = this;

  // If there is a currently selected item we need to clear its selected state
  if(old_selection) {
    if(old_selection->content_container_) old_selection->content_container_->SetVisibility(Visibility::Collapsed);
    old_selection->UpdateAnimationState();
  }

  // Parent the new selected content
  if(content_container_) content_container_->SetVisibility(Visibility::Visible);

  tc_->SelectionChanged(tc_);

  UpdateAnimationState();
}

void TabItem::UpdateAnimationState (void) {
  ContentControl::UpdateAnimationState();

  // Selected state
  if(tc_&&tc_->selected_==this) {
    if(tc_->active_) {      
      SetAnimationState(UI::StateSelected);
    } else {
      SetAnimationState(UI::StateSelectedUnFocused);
    }
  } else{
    SetAnimationState(UI::StateUnSelected);
  }

  // Focused state
  //SetAnimationState(IsFocused() ? UI::StateFocused : UI::StateUnFocused);
}

void TabItem::OnChildAdded(Node * child) {
  ContentControl::OnChildAdded(child);

  noz_assert(content_container_);
  content_container_->AddChild(child);
}

TabItem* TabItem::GetNextSiblingItem (void) const {
  for(Node* n=GetNextLogicalSibling(); n; n=n->GetNextLogicalSibling()) {
    if(n->IsTypeOf(typeof(TabItem))) return (TabItem*)n;
  }
  return nullptr;
}

TabItem* TabItem::GetPrevSiblingItem (void) const {
  for(Node* n=GetPrevLogicalSibling(); n; n=n->GetPrevLogicalSibling()) {
    if(n->IsTypeOf(typeof(TabItem))) return (TabItem*)n;
  }
  return nullptr;
}

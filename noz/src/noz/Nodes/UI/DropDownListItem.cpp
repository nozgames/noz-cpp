///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "DropDownListItem.h"
#include "DropDownList.h"

using namespace noz;


DropDownListItem::DropDownListItem (void) {
  drop_down_list_ = nullptr;
  user_data_= nullptr;
}

bool DropDownListItem::IsSelected (void) const {
  return drop_down_list_ && GetLogicalIndex() == drop_down_list_->selected_item_index_;
}

void DropDownListItem::Select (void) {  
  if(drop_down_list_) drop_down_list_->SetSelectedItemIndex(GetLogicalIndex());
}

void DropDownListItem::OnMouseDown (SystemEvent* e) {
  noz_assert(e);

  if(nullptr == drop_down_list_) return;

  // Left mouse button?
  if(e->GetButton() != MouseButton::Left) return;

  // Double click?
  if(e->GetClickCount() != 1) return;

  // Set focus to the drop down list
  drop_down_list_->SetFocus();

  // Mark event as handled
  e->SetHandled();

  // Select the item if not selected
  if(!IsSelected()) Select ();

  // Always commit the selection to close the popup
  drop_down_list_->CommitSelection();
}

void DropDownListItem::UpdateAnimationState (void) {
  // Selected state
  if(IsSelected()) {
    SetAnimationState(UI::StateSelected);
  } else{
    SetAnimationState(UI::StateUnSelected);
  }

  Control::UpdateAnimationState();
}

DropDownListItem* DropDownListItem::GetNextSiblingItem (void) const {
  for(Node* n=GetNextLogicalSibling(); n; n=n->GetNextLogicalSibling()) {
    if(n->IsTypeOf(typeof(DropDownListItem))) return (DropDownListItem*)n;
  }
  return nullptr;
}

DropDownListItem* DropDownListItem::GetPrevSiblingItem (void) const {
  for(Node* n=GetPrevLogicalSibling(); n; n=n->GetPrevLogicalSibling()) {
    if(n->IsTypeOf(typeof(DropDownListItem))) return (DropDownListItem*)n;
  }
  return nullptr;
}

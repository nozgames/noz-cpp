///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Render/TextNode.h>
#include "Button.h"
#include "DockPane.h"
#include "DockItem.h"
#include "DockTabItem.h"
#include "DockManager.h"
#include "TabControl.h"

using namespace noz;

DockPane::DockPane(void) {
  manager_ = nullptr;
  active_  = false;
  dock_style_ = DockStyle::Left;
  SetLogicalChildrenOnly();
  Application::FocusChanged += FocusChangedEventHandler::Delegate(this,&DockPane::OnFocusChanged);
}

DockItem* DockPane::GetActiveItem (void) const {
  DockTabItem* tab = GetActiveTab();
  if(nullptr == tab) return nullptr;
  return tab->item_;
}

DockTabItem* DockPane::GetActiveTab (void) const {
  if(nullptr == tab_control_) return nullptr;
  return Cast<DockTabItem>(tab_control_->GetSelected());
}

void DockPane::OnTabSelectionChanged (UINode* sender) {
  noz_assert(tab_control_);

  // Get selected item
  TabItem* selected = tab_control_->GetSelected();
  if(nullptr==selected) {
    return;
  }  
}

bool DockPane::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == tab_control_) return false;

  tab_control_->SelectionChanged += SelectionChangedEventHandler::Delegate(this, &DockPane::OnTabSelectionChanged);

  // Add all of the dock items to the tab control within the pane.
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    DockItem* item = Cast<DockItem>(GetLogicalChild(i));
    if(item) tab_control_->AddChild(item->tab_);
  }

  return true;
}

void DockPane::OnStyleChanged(void) {
  Control::OnStyleChanged();

  // Remove all of the items from the tab control
  for(noz_uint32 i=GetLogicalChildCount(); i>0; i--) {
    DockItem* item = Cast<DockItem>(GetLogicalChild(i-1));
    if (item) item->tab_->Orphan();
  }

  tab_control_ = nullptr;
}

void DockPane::OnCloseButton (UINode* sender) {
  DockItem* item = GetActiveItem();
  if(nullptr == item) return;
  item->Hide();
}

void DockPane::OnMouseDown (SystemEvent* e) {
  Control::OnMouseDown(e);

  e->SetHandled();
  SetFocus();
}

void DockPane::UpdateAnimationState (void) {
  Control::UpdateAnimationState();

  if(active_) {
    SetAnimationState(UI::StateFocused);
  } else {
    SetAnimationState(UI::StateUnFocused);
  }
}

void DockPane::OnFocusChanged(Window* window, UINode* new_focus) {
  // Dont care about the focus of another window
  if(window != GetWindow()) return;

  if(new_focus && (new_focus == this || new_focus->IsDescendantOf(this))) {
    if(active_==true) return;
    active_ = true;
  } else {
    if(active_==false) return;
    active_ = false;
  }
  UpdateAnimationState();
}

void DockPane::InvalidateDock (void) {
  manager_->InvalidateDock();
}

void DockPane::SetDock (DockStyle style) {
  if(dock_style_ == style) return;
  dock_style_ = style;
}

void DockPane::OnChildAdded(Node* child) {
  Control::OnChildAdded(child);

  DockItem* item = Cast<DockItem>(child);

  if(tab_control_ && item) {
    tab_control_->AddChild(item->tab_);
  }

  if(item) item->pane_ = this;
}


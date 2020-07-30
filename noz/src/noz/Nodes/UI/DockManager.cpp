///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Render/TextNode.h>
#include "DockManager.h"
#include "DockItem.h"
#include "DockPane.h"
#include "DockTabItem.h"
#include "TabControl.h"
#include "TabItem.h"
#include "Splitter.h"

using namespace noz;


DockManager::DockManager(void) {
  dock_invalid_ = true;
  SetLogicalChildrenOnly();
}

DockManager::~DockManager(void) {
}

bool DockManager::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  DockPanes ();
  return true;
}

void DockManager::OnStyleChanged (void) {
  Control::OnStyleChanged();

  UnDockPanes ();

  panes_container_ = nullptr;
}

Splitter* DockManager::CreateSplitter (DockStyle style) {
  Splitter* splitter = new Splitter;
  splitter->AddChild (new DockThumb);

  switch(style) {
    case DockStyle::Left:
      splitter->SetSplitAlignment(Alignment::Min);
      splitter->SetOrientation(Orientation::Horizontal);
      break;

    case DockStyle::Top:
      splitter->SetSplitAlignment(Alignment::Min);
      splitter->SetOrientation(Orientation::Vertical);
      break;

    case DockStyle::Right:
      splitter->SetSplitAlignment(Alignment::Max);
      splitter->SetOrientation(Orientation::Horizontal);
      break;

    case DockStyle::Bottom:
      splitter->SetSplitAlignment(Alignment::Max);
      splitter->SetOrientation(Orientation::Vertical);
      break;
      
    default: break;
  }

  splitter->SetSplit(300);

  return splitter;
}

/*
DockPane* DockManager::CreatePane (DockItem* item) {
  noz_assert(item);

  // Create the pane..
  DockPane* pane = new DockPane;
  pane->manager_ = this;

  item->tab_ = CreateTab (item,pane);

  // Update the pane title to reflect the first tab
  if(pane->tabs_->GetItemCount()==1 && pane->title_node_) {
    pane->title_node_->SetText(item->title_);
  }

  return pane;
}

DockTabItem* DockManager::CreateTab (DockItem* item, DockPane* pane) {
  noz_assert(item);
  noz_assert(pane);
  noz_assert(pane->tabs_);

  // Create the tab item
  item->tab_ = new DockTabItem;

  item->pane_ = pane;

  // Set the content
  item->tab_->SetContent(item->content_);

  // Add the tab to the pane
  pane->tabs_->AddItem((DockTabItem*)item->tab_);

  // Associate the item with the tab.
  item->tab_->item_ = item;

  if(item->tab_ && item->tab_->title_node_) {
    item->tab_->title_node_->SetText(item->title_);
  }

  return item->tab_;
}
*/

void DockManager::DockPanes (void) {
  if(panes_container_ == nullptr) return;

//  DockItem* previously_selected = selected_;
//  selected_ = nullptr;

  UnDockPanes ();

  // Create a new root for all docked items
  DockPane* pane = nullptr;
  Splitter* splitter = nullptr;
  noz_int32 splitter_content = 0;

  // Organize the children as docks.
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    DockPane* pane = Cast<DockPane>(GetLogicalChild(i));

    // If the child is not a pane then it is undocked content..
    if(nullptr == pane) {
      if(content_container_ == nullptr) content_container_ = new Node;
      content_container_->AddChild(GetLogicalChild(i));
      continue;
    }

    // Skip collapsed panes
    if(pane->GetVisibility() == Visibility::Collapsed) continue;

    // Skip panes that have no visible items
    bool pane_visible = false;
    for(noz_uint32 ii=0,cc=pane->GetLogicalChildCount();!pane_visible&&ii<cc;ii++) {
      pane_visible = pane->GetLogicalChild(ii)->GetVisibility() == Visibility::Visible;
    }      
    if(!pane_visible) continue;
   
    switch(pane->GetDock()) {
      case DockStyle::Left:
      case DockStyle::Top:  {
        // Create a new splitter and add it to the current parent
        Splitter* new_splitter = CreateSplitter(pane->GetDock());

        if(splitter) {
          if(splitter_content==0) {
            splitter->InsertChild(0,new_splitter);
          } else {
            splitter->AddChild(new_splitter);
          }
        } else {
          panes_container_->AddChild(new_splitter);
        }

        splitter = new_splitter;
        splitter_content = 1;

        Node* insulator = new Node;
        insulator->AddChild(pane);
        splitter->InsertChild (0, insulator);

        break;
      }

      case DockStyle::Right: 
      case DockStyle::Bottom: {
        // Create a new splitter and add it to current parent
        Splitter* new_splitter = CreateSplitter(pane->GetDock());

        if(splitter) {
          if(splitter_content==0) {
            splitter->InsertChild(0,new_splitter);
          } else {
            splitter->AddChild(new_splitter);
          }
        } else {
          panes_container_->AddChild(new_splitter);
        }
          
        splitter = new_splitter;
        splitter_content = 0;

        Node* insulator = new Node;
        insulator->AddChild(pane);
        splitter->AddChild(insulator);

        break;
      }
      
      default:
        noz_assert(false);
        break;
    }
  }

  // Parent the undocked content container to the appropriate splitter
  if(content_container_->GetChildCount()) {
    if(splitter) {
      if(splitter_content==0) {
        splitter->InsertChild(0,content_container_);
      } else {
        splitter->AddChild(content_container_);
      }
    } else {
      panes_container_->AddChild(content_container_);
    }
  }
    
/*
  // Reselect what was previously selected
  if(previously_selected && previously_selected->GetPane()) {
    previously_selected->Select();
  }
*/
}

void DockManager::UnDockPanes(void) {
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) GetLogicalChild(i)->Orphan(false);

  if(content_container_) content_container_->Orphan();
  
  dock_invalid_ = false;
  undocked_root_ = nullptr;
}

Vector2 DockManager::Measure (const Vector2& a) {
  // Adjust the dock if the dock is invalid.
  if(dock_invalid_) DockPanes ( );

  return Control::Measure(a);  
}

void DockManager::InvalidateDock(void) {
  dock_invalid_ = true;
  InvalidateTransform();
}


void DockManager::Select(DockPane* pane) {
  if(pane && pane->manager_ != this) return;

  DockPane* was_selected = selected_;
  selected_ = pane;
  if(was_selected) was_selected->UpdateAnimationState();
  if(selected_) selected_->UpdateAnimationState();
}

void DockManager::OnChildAdded(Node* child) {
  Control::OnChildAdded(child);

  if(child->IsTypeOf(typeof(DockPane))) {
    ((DockPane*)child)->manager_ = this;
    InvalidateDock();
  } else if(content_container_) {
    content_container_->AddChild(child);
  } else {
    InvalidateDock();
  }
}

void DockManager::OnChildRemoved(Node* child) {
  Control::OnChildRemoved(child);

  if(child->IsTypeOf(typeof(DockPane))) {
    ((DockPane*)child)->manager_ = nullptr;
    InvalidateDock();
  }
}


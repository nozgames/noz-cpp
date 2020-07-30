///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/ToggleButton.h>
#include <noz/Nodes/Render/TextNode.h>
#include "AssetBrowser.h"
#include "AssetBrowserTreeViewItem.h"

using namespace noz;
using namespace noz::Editor;


AssetBrowserTreeViewItem::AssetBrowserTreeViewItem (void) {
  folder_ = nullptr;
  SetDragDropTarget(true);
}

AssetBrowserTreeViewItem::~AssetBrowserTreeViewItem(void) {
}

bool AssetBrowserTreeViewItem::OnApplyStyle(void) {
  if(!TreeViewItem::OnApplyStyle()) return false;

  if(folder_) SetText(folder_->GetName());

  if(GetParentItem()==nullptr) {
    if(expand_button_) expand_button_->SetVisibility(Visibility::Hidden);
  }

  return true;
}

void AssetBrowserTreeViewItem::SetFolder(AssetFolder* folder) {
  if(folder==folder_) return;
  folder_ = folder;
  if(folder_) SetText(folder_->GetName());
}

void AssetBrowserTreeViewItem::OnDragDrop (DragDropEventArgs* args) {
  switch(args->GetEventType()) {
    case DragDropEventType::Enter: {
      if(args->GetObject<AssetFile>() || args->GetObject<AssetFolder>()) {
        args->SetEffects(DragDropEffects::Move);
        SetAnimationState(UI::StateDragDropInto);
        return;
      }
      break;
    }

    case DragDropEventType::Leave:
      SetAnimationState(UI::StateDragDropNone);
      break;

    case DragDropEventType::Drop:
      SetAnimationState(UI::StateDragDropNone);
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Render/TextNode.h>
#include <noz/Nodes/Render/SpriteNode.h>
#include "AssetBrowserListViewItem.h"
#include "AssetBrowser.h"

using namespace noz;
using namespace noz::Editor;

AssetBrowserListViewItem::AssetBrowserListViewItem (AssetBrowser* browser, AssetFolder* folder) {
  file_ = nullptr;
  folder_ = folder;
  browser_ = browser;
}

AssetBrowserListViewItem::AssetBrowserListViewItem (AssetBrowser* browser, AssetFile* file) {
  file_ = file;
  folder_ = nullptr;
  browser_ = browser;
}

AssetBrowserListViewItem::~AssetBrowserListViewItem(void) {
}

bool AssetBrowserListViewItem::OnApplyStyle(void) {
  if(!ListViewItem::OnApplyStyle()) return false;

  if(folder_) {
    SetText(folder_->GetName());

    // Set icon based on the list view type
    if(browser_->list_view_zoom_==0.0f) {
      SetSprite(AssetManager::LoadAsset<Sprite>(Guid::Parse("{ACE6FAFE-88B4-4DF2-B6CF-F83956F4711A}")));
    } else {
      SetSprite(AssetManager::LoadAsset<Sprite>(Guid::Parse("{1D836B0F-942B-4221-9A6E-AE1756BCC87C}")));
    }
  } else {
    noz_assert(file_);

    // Set text to file name
    SetText(file_->GetName());

    // Set icon based on the list view type
    if(browser_->list_view_zoom_==0.0f) {
      SetSprite(EditorFactory::CreateTypeIcon(file_->GetAssetType()));
    } else {
      SetSprite(AssetManager::LoadAsset<Sprite>(Guid::Parse("{67F1C13B-618A-4731-9BC5-53D141DC52AF}")));
    }
  }

  return true;
}

void AssetBrowserListViewItem::OnMouseDown (SystemEvent* e) { 
  ListViewItem::OnMouseDown(e);

  // Must be left mouse button..
  if(e->GetButton() != MouseButton::Left) return;

  if(e->GetClickCount() != 2) {
    SetCapture();
    return;
  }

  OnOpen();

  e->SetHandled();
}

void AssetBrowserListViewItem::OnMouseOver (SystemEvent* e) { 
  ListViewItem::OnMouseOver(e);

  NOZ_TODO("Use UINode drag drop code")
  if(HasCapture() && Input::GetMouseButtonDragDelta(MouseButton::Left).length_sqr() > 25.0f) {
    DragDrop::DoDragDrop(this, file_, DragDropEffects::Move);
  }
}

void AssetBrowserListViewItem::OnMouseUp (SystemEvent* e) { 
  ListViewItem::OnMouseUp(e);
  ReleaseCapture();
}

void AssetBrowserListViewItem::OnOpen (void) {
  if(folder_) {
    browser_->SelectFolder(folder_);
  } else if (file_) {
    EditorApplication::EditAsset(file_);
  }  
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AssetPicker.h"

#include <noz/Nodes/UI/ListView.h>
#include <noz/Nodes/UI/ListViewItem.h>
#include <noz/Nodes/UI/TextBox.h>
#include <noz/Nodes/UI/Button.h>

using namespace noz;
using namespace noz::Editor;

AssetPicker::AssetPicker (void) {
}

AssetPicker::~AssetPicker(void) {
}

bool AssetPicker::OnApplyStyle(void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == list_view_) return false;

  if(text_box_) {
    text_box_->TextChanged += ValueChangedEventHandler::Delegate(this,&AssetPicker::OnSearchTextChanged);
  }

  if(none_button_) none_button_->Click += ClickEventHandler::Delegate(this,&AssetPicker::OnNoneButtonClicked);

  list_view_->SelectionChanged += SelectionChangedEventHandler::Delegate(this, &AssetPicker::OnListViewSelectionChanged);

  Refresh();

  if(text_box_) {
    text_box_->SetText("");
    text_box_->SetFocus();
  }

  return true;
}

void AssetPicker::Clear(void) {
  SetFilterText("");
  if(list_view_) list_view_->UnselectAll();
}

void AssetPicker::OnLineageChanged(void) {
  Control::OnLineageChanged();

  if(GetWindow()) {
    Refresh();
    if(text_box_) {
      text_box_->SetText("");
      text_box_->SetFocus();
    }
  }
}

void AssetPicker::SetFilterText (const char* text) {
  if(filter_text_.Equals(text,StringComparison::OrdinalIgnoreCase)) return;
  filter_text_ = text;
  Refresh();
}

void AssetPicker::Refresh (void) {
  if(list_view_ == nullptr) return;
  if(IsOrphan()) return;

  list_view_->RemoveAllChildren();

  std::vector<AssetFile*> files = AssetDatabase::GetFiles(typeof(Sprite),filter_text_.IsEmpty() ? nullptr : filter_text_.ToCString());
  for(noz_uint32 i=0,c=files.size(); i<c; i++) {
    AssetFile* file = files[i];
    noz_assert(file);

    ListViewItem* item = new ListViewItem;
    item->SetUserData(file);
    item->SetText(file->GetName());
    item->SetSprite(AssetManager::LoadAsset<Sprite>(Guid::Parse("{35FE884D-FE81-4D24-A7FB-EB212E0E26C5}")));
    list_view_->AddChild(item);
  }
}

void AssetPicker::OnListViewSelectionChanged (UINode* sender) {
  if(list_view_->GetSelectedItem()==nullptr) return;

  AssetFile* file = (AssetFile*)list_view_->GetSelectedItem()->GetUserData();;
  if(file==nullptr) {
    SelectionChanged(this,nullptr);
  } else {
    SelectionChanged(this,AssetManager::LoadAsset(file->GetAssetType(),file->GetGuid()));
  }
  if(GetWindow() && GetWindow()->GetPopup()) {
    SetFocus();
    GetWindow()->GetPopup()->Close();
  }
}

void AssetPicker::OnSearchTextChanged (UINode* sender) {
  filter_text_ = text_box_->GetText();
  Refresh();
}

void AssetPicker::OnNoneButtonClicked (UINode* sender) {
  SelectionChanged(this,nullptr);  
  if(GetWindow() && GetWindow()->GetPopup()) {
    SetFocus();
    GetWindow()->GetPopup()->Close();
  }
}

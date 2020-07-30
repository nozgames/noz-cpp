///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TypePicker.h"

#include <noz/Nodes/UI/ListView.h>
#include <noz/Nodes/UI/ListViewItem.h>
#include <noz/Nodes/UI/TextBox.h>

using namespace noz;
using namespace noz::Editor;

TypePicker::TypePicker (void) {
  base_type_ = typeof(Object);
  filter_editor_only_ = true;
  filter_no_allocator_ = true;
}

TypePicker::~TypePicker(void) {
}

bool TypePicker::OnApplyStyle(void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == list_view_) return false;

  if(text_box_) {
    text_box_->TextChanged += ValueChangedEventHandler::Delegate(this,&TypePicker::OnSearchTextChanged);
    text_box_->SetText(filter_text_.ToCString());
  }

  list_view_->SelectionChanged += SelectionChangedEventHandler::Delegate(this,&TypePicker::OnListViewSelectionChanged);  

  Refresh();

  return true;
}

void TypePicker::Clear(void) {
  SetFilterText("");
  if(list_view_) list_view_->UnselectAll();
}

void TypePicker::SetFilterText (const char* text) {
  if(filter_text_.Equals(text,StringComparison::OrdinalIgnoreCase)) return;
  filter_text_ = text;
  if(text_box_) text_box_->SetText(text);
  Refresh();
}

void TypePicker::SetBaseType (Type* type) {
  if(type==nullptr) type = typeof(Object);
  if(base_type_ == type) return;
  base_type_ = type;
  Refresh();
}

int TypePickerSortProc (const Type* lhs, const Type* rhs) {
  return lhs->GetName().ToString().CompareTo(rhs->GetName().ToString()) < 0;
}

void TypePicker::Refresh (void) {
  if(nullptr == list_view_) return;

  if(types_.empty()) {
    types_ = Type::GetTypes(base_type_,nullptr);

    std::sort(types_.begin(), types_.end(), &TypePickerSortProc);
  }

  list_view_->RemoveAllChildren();

  for(auto it=types_.begin(); it!=types_.end(); it++) {
    Type* t = *it;
    noz_assert(t);

    if(filter_editor_only_ && t->IsEditorOnly()) continue;
    if(filter_no_allocator_ && !t->HasAllocator()) continue;
    if(!filter_text_.IsEmpty() && -1==t->GetName().ToString().IndexOf(filter_text_.ToCString(),0,StringComparison::OrdinalIgnoreCase)) continue;

    TypePickerItem* item = new TypePickerItem;
    item->SetUserData(t);
    item->SetText(t->GetName());
    item->SetSprite(EditorFactory::CreateTypeIcon(t));
    list_view_->AddChild(item);
  }
}

void TypePicker::OnListViewSelectionChanged (UINode* sender) {
  if(list_view_->GetSelectedItem()) {
    SelectionChanged(this, ((Type*)list_view_->GetSelectedItem()->GetUserData()));
  }

/*
  AssetFile* file = (AssetFile*)list_view_->GetSelectedItem()->GetUserData();;
  if(file==nullptr) {
    AssetSelected(this,nullptr);
  } else {
    AssetSelected(this,AssetManager::LoadAsset(file->GetAssetType(),file->GetGuid()));
  }
  if(GetWindow() && GetWindow()->GetPopup()) {
    SetFocus();
    GetWindow()->GetPopup()->Close();
  }
  */
}

void TypePicker::OnSearchTextChanged (UINode* sender) {
  filter_text_ = text_box_->GetText();
  Refresh();
}

Type* TypePicker::GetSelected (void) const {
  if(list_view_ && list_view_->GetSelectedItem()) {
    return (Type*)list_view_->GetSelectedItem()->GetUserData();
  }
  return nullptr;
}

void TypePicker::OnGainFocus(void) {
  if(text_box_) text_box_->SetFocus();
}

TypePickerItem::TypePickerItem(void) {
  SetDragDropSource(true);
}

void TypePickerItem::DoDragDrop (void) {
  DragDrop::DoDragDrop(this, this, DragDropEffects::Copy);
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Actions/SetPropertyAction.h>
#include <noz/Editor/Nodes/UI/AnimationView.h>
#include "EnumPropertyEditor.h"


using namespace noz;
using namespace noz::Editor;

EnumPropertyEditor::EnumPropertyEditor(void) {
}

void EnumPropertyEditor::OnSetTarget (void) {
  if(drop_down_list_) drop_down_list_->RemoveAllChildren();
}

bool EnumPropertyEditor::OnApplyStyle (void) {
  if(!PropertyEditor::OnApplyStyle()) return false;
  if(nullptr == drop_down_list_) return false;

  drop_down_list_->SelectionChanged += ValueChangedEventHandler::Delegate(this, &EnumPropertyEditor::OnDropDownListSelectionChanged);

  UpdateDropDownList();

  return true;
}

const Name& EnumPropertyEditor::GetValue (void) const {
  if(nullptr==drop_down_list_) return Name::Empty;
  DropDownListItem* item = drop_down_list_->GetSelectedItem();
  if(nullptr == item) return Name::Empty;
  return *((const Name*)item->GetUserData());
}

void EnumPropertyEditor::SetValue (const Name& v) {
  // Find the enum value name within the drop down list.
  for(DropDownListItem* item=drop_down_list_->GetFirstChildItem(); item; item=item->GetNextSiblingItem()) {
    const Name* enum_value_name = (const Name*) item->GetUserData();
    noz_assert(enum_value_name);
    if(*enum_value_name == v) {
      item->Select();
      return;
    }
  }
}
void EnumPropertyEditor::WriteProperty (Object* t, Property* p) {
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::EnumProperty)));
  noz_assert(Workspace::GetWorkspace(this)->GetActiveDocument());

  if(nullptr==drop_down_list_) return;

  DropDownListItem* item = drop_down_list_->GetSelectedItem();
  if(nullptr == item) return;

  Workspace::GetWorkspace(this)->GetActiveDocument()->ExecuteAction(new SetEnumPropertyAction(t,p,*((const Name*)item->GetUserData())));
}

void EnumPropertyEditor::ReadProperty (Object* t, Property* p) {  
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::EnumProperty)));

  if(nullptr == drop_down_list_) return;

  UpdateDropDownList();
  SetValue(((EnumProperty*)p)->Get(t));
}

void EnumPropertyEditor::OnDropDownListSelectionChanged (UINode* sender) {
  if(!IsReadingProperty()) {
    PropertyEditor::WriteProperty();
  }
}

void EnumPropertyEditor::UpdateDropDownList (void) {
  if(GetTargetProperty()==nullptr) return;
  if(drop_down_list_ == nullptr) return;
  if(drop_down_list_->GetLogicalChildCount()>0) return;

  const std::vector<Name>& names = ((EnumProperty*)GetTargetProperty())->GetNames();
  for(noz_int32 i=0,c=names.size(); i<c; i++) {
    DropDownListItem* item = new DropDownListItem;
    item->SetText(names[i]);
    item->SetUserData((void*)&names[i]);
    drop_down_list_->AddChild(item);
  }
}

void EnumPropertyEditor::Update (void) {
  PropertyEditor::Update();

  if(nullptr==drop_down_list_) return;
  if(nullptr==GetTarget()) return;
  if(nullptr==GetTargetProperty()) return;
  
  noz_assert(GetTargetProperty()->IsType(typeof(noz::EnumProperty)));

  // Look for a change in color..
  const Name& current = ((EnumProperty*)GetTargetProperty())->Get(GetTarget());
  
  if(current != GetValue()) {
    reading_property_ = true;
    SetValue(current);
    reading_property_ = false;
  }
}


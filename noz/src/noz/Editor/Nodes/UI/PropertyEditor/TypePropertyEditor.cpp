///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TypePropertyEditor.h"


using namespace noz;
using namespace noz::Editor;

TypePropertyEditor::TypePropertyEditor(void) {
  type_ = nullptr;
}

bool TypePropertyEditor::OnApplyStyle (void) {
  if(!PropertyEditor::OnApplyStyle()) return false;
  if(nullptr == type_picker_) return false;
  if(nullptr == button_) return false;

  button_->Click += ClickEventHandler::Delegate(this,&TypePropertyEditor::OnButtonClick);
  type_picker_->SelectionChanged += TypeSelectedEventHandler::Delegate(this,&TypePropertyEditor::OnTypeSelected);

  if(type_) {
    button_->SetText(type_->GetName());
    button_->SetSprite(EditorFactory::CreateTypeIcon(type_));
  }

  return true;
}

void TypePropertyEditor::WriteProperty (Object* t, Property* p) {
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::TypeProperty)));

  ((TypeProperty*)p)->Set(t,type_);
}

void TypePropertyEditor::ReadProperty (Object* t, Property* p) {  
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::TypeProperty)));

  type_ = ((TypeProperty*)p)->Get(t);

  if(button_) {
    if(type_) {
      button_->SetText(type_->GetName());
      button_->SetSprite(EditorFactory::CreateTypeIcon(type_));
    } else {
      button_->SetText("None");
      button_->SetSprite(nullptr);
    }
  }
}

void TypePropertyEditor::OnButtonClick (UINode* sender) {
  if(nullptr == popup_ || nullptr==type_picker_) return;

  if(popup_) {
    Type* type = Type::FindType(GetTargetProperty()->GetMeta("EditorFilterType", "noz::Object"));
    if(nullptr == type) {
      type = typeof(Object);
    }
    type_picker_->SetBaseType(type);
    popup_->Open();
  }
}

void TypePropertyEditor::OnTypeSelected (UINode* sender, Type* type) {
  type_ = type;

  if(button_ && type_) {
    button_->SetText(type_->GetName());
    button_->SetSprite(EditorFactory::CreateTypeIcon(type_));
  }

  popup_->Close();

  PropertyEditor::WriteProperty();
}


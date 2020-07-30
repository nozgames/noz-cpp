///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include <noz/Nodes/UI/TextBox.h>
#include <noz/Nodes/UI/Button.h>
#include "LayoutLengthPropertyEditor.h"


using namespace noz;
using namespace noz::Editor;

LayoutLengthPropertyEditor::LayoutLengthPropertyEditor(void) {
}

bool LayoutLengthPropertyEditor::OnApplyStyle (void) {  
  if(nullptr == text_) return false;
  if(nullptr == auto_button_) return false;

  text_->TextCommited += ValueChangedEventHandler::Delegate(this, &LayoutLengthPropertyEditor::OnTextBoxTextChanged);
  auto_button_->Click += ClickEventHandler::Delegate(this, &LayoutLengthPropertyEditor::OnAutoButtonClicked);

  return PropertyEditor::OnApplyStyle();
}

void LayoutLengthPropertyEditor::WriteProperty (Object* t, Property* p) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::LayoutLengthProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  LayoutLength v;
  if(text_) {
    String s = text_->GetText().Trim();
    if(s.Equals("Auto",StringComparison::OrdinalIgnoreCase)) {
      v = LayoutLength(LayoutUnitType::Auto,0.0f);
    } else if(!s.IsEmpty() && s[s.GetLength()-1] == '%') {
      v = LayoutLength(LayoutUnitType::Percentage, Float::Parse(s.Substring(0,s.GetLength()-1)));
    } else {
      v = LayoutLength(LayoutUnitType::Fixed , Float::Parse(s));
    }
  }

  ((LayoutLengthProperty*)p)->Set(t,v);
}

void LayoutLengthPropertyEditor::ReadProperty (Object* t, Property* p) {  
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::LayoutLengthProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  LayoutLength v = ((LayoutLengthProperty*)p)->Get(t);

  if(text_) {
    if(v.unit_type_ == LayoutUnitType::Percentage) {  
      text_->SetText(String::Format("%8.3g%%",v.value_).Trim());
    } else if (v.unit_type_ == LayoutUnitType::Auto) {
      text_->SetText("Auto");
    } else {
      text_->SetText(String::Format("%8.3g",v.value_).Trim());
    }
  }
}

void LayoutLengthPropertyEditor::OnTextBoxTextChanged(UINode*) {
  PropertyEditor::WriteProperty();
}

void LayoutLengthPropertyEditor::OnAutoButtonClicked (UINode*) {
  if(text_) text_->SetText("Auto");
  PropertyEditor::WriteProperty();
}

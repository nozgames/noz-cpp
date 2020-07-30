///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/CheckBox.h>
#include "BoolPropertyEditor.h"


using namespace noz;
using namespace noz::Editor;

BoolPropertyEditor::BoolPropertyEditor(void) {
}

bool BoolPropertyEditor::OnApplyStyle (void) {
  if(nullptr == check_box_) return false;
  
  check_box_->Click += ClickEventHandler::Delegate(this, &BoolPropertyEditor::OnCheckBoxClick);

  return PropertyEditor::OnApplyStyle();
}

void BoolPropertyEditor::WriteProperty (Object* t, Property* p) {
  if(check_box_) ((BooleanProperty*)p)->Set(t,check_box_->IsChecked());
}

void BoolPropertyEditor::ReadProperty (Object* t, Property* p) {  
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::BooleanProperty)));
  if(check_box_) check_box_->SetChecked(((BooleanProperty*)p)->Get(t));
}

void BoolPropertyEditor::OnCheckBoxClick (UINode* sender) {
  PropertyEditor::WriteProperty();
}

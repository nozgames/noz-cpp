///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RectPropertyEditor.h"

#include <noz/Nodes/UI/TextBox.h>

using namespace noz;
using namespace noz::Editor;

RectPropertyEditor::RectPropertyEditor(void) {
}

bool RectPropertyEditor::OnApplyStyle (void) {
  if(!PropertyEditor::OnApplyStyle()) return false;
  if(nullptr == text_box_x_) return false;
  if(nullptr == text_box_y_) return false;
  if(nullptr == text_box_w_) return false;
  if(nullptr == text_box_h_) return false;

  text_box_x_->TextCommited += ValueChangedEventHandler::Delegate(this, &RectPropertyEditor::OnTextBoxTextChanged);
  text_box_y_->TextCommited += ValueChangedEventHandler::Delegate(this, &RectPropertyEditor::OnTextBoxTextChanged);
  text_box_w_->TextCommited += ValueChangedEventHandler::Delegate(this, &RectPropertyEditor::OnTextBoxTextChanged);
  text_box_h_->TextCommited += ValueChangedEventHandler::Delegate(this, &RectPropertyEditor::OnTextBoxTextChanged);

  return true;
}

void RectPropertyEditor::WriteProperty (Object* t, Property* p) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::RectProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  Rect r;
  if(text_box_x_) r.x=Float::Parse(text_box_x_->GetText());
  if(text_box_y_) r.y=Float::Parse(text_box_y_->GetText());
  if(text_box_w_) r.w=Float::Parse(text_box_w_->GetText());
  if(text_box_h_) r.h=Float::Parse(text_box_h_->GetText());
  ((RectProperty*)p)->Set(t,r);
}

void RectPropertyEditor::ReadProperty (Object* t, Property* p) {  
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::RectProperty);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  Rect r = ((RectProperty*)p)->Get(t);
  if(text_box_x_) text_box_x_->SetText(String::Format("%-8.3g",r.x).Trim());
  if(text_box_y_) text_box_y_->SetText(String::Format("%-8.3g",r.y).Trim());
  if(text_box_w_) text_box_w_->SetText(String::Format("%-8.3g",r.w).Trim());
  if(text_box_h_) text_box_h_->SetText(String::Format("%-8.3g",r.h).Trim());
}

void RectPropertyEditor::OnTextBoxTextChanged(UINode*) {
  PropertyEditor::WriteProperty();
}

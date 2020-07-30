///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Actions/SetPropertyAction.h>
#include "Vector2PropertyEditor.h"

#include <noz/Nodes/UI/TextBox.h>

using namespace noz;
using namespace noz::Editor;

Vector2PropertyEditor::Vector2PropertyEditor(void) {
}

bool Vector2PropertyEditor::OnApplyStyle (void) {
  if(!PropertyEditor::OnApplyStyle()) return false;
  if(nullptr == text_box_x_) return false;
  if(nullptr == text_box_y_) return false;

  text_box_x_->TextCommited += ValueChangedEventHandler::Delegate(this, &Vector2PropertyEditor::OnTextBoxTextChanged);
  text_box_y_->TextCommited += ValueChangedEventHandler::Delegate(this, &Vector2PropertyEditor::OnTextBoxTextChanged);

  return true;
}

void Vector2PropertyEditor::WriteProperty (Object* t, Property* p) {
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::Vector2Property);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  if(!IsReadingProperty()) {
    EditorDocument::GetActiveDocument(this)->ExecuteAction(new SetVector2PropertyAction(t,p,value_));
  }
}

void Vector2PropertyEditor::ReadProperty (Object* t, Property* p) {  
  noz_assert(p);
  noz_assert(t);
  noz_assert(p->GetType()->GetTypeCode() == TypeCode::Vector2Property);
  noz_assert(t->IsTypeOf(p->GetParentType()));

  value_ = ((Vector2Property*)p)->Get(t);
  if(text_box_x_) text_box_x_->SetText(Float(value_.x).ToString());
  if(text_box_y_) text_box_y_->SetText(Float(value_.y).ToString());
}

void Vector2PropertyEditor::OnTextBoxTextChanged(UINode*) {
  if(IsReadingProperty()) return;
  if(text_box_x_) value_.x=Float::Parse(text_box_x_->GetText());
  if(text_box_y_) value_.y=Float::Parse(text_box_y_->GetText());
  PropertyEditor::WriteProperty();
}

void Vector2PropertyEditor::Update (void) {
  PropertyEditor::Update();

  if(nullptr==GetTarget()) return;
  if(nullptr==GetTargetProperty()) return;
  
  noz_assert(GetTargetProperty()->IsType(typeof(noz::Vector2Property)));

  // Look for a change in the vector..
  Vector2 current = ((Vector2Property*)GetTargetProperty())->Get(GetTarget());
  if(current != value_) {
    PropertyEditor::ReadProperty();
  }
}


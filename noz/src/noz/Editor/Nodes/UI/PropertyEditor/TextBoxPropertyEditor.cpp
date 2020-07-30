///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TextBoxPropertyEditor.h"

#include <noz/Nodes/UI/TextBox.h>

using namespace noz;
using namespace noz::Editor;

TextBoxPropertyEditor::TextBoxPropertyEditor(void) {
}

bool TextBoxPropertyEditor::OnApplyStyle (void) {
  if(!PropertyEditor::OnApplyStyle()) return false;
  if(nullptr == text_box_) return false;

  text_box_->TextCommited += ValueChangedEventHandler::Delegate(this, &TextBoxPropertyEditor::OnTextBoxTextChanged);

  return true;
}

void TextBoxPropertyEditor::WriteProperty (Object* target, Property* prop) {
  WriteString(target,prop,text_box_->GetText());
}

void TextBoxPropertyEditor::ReadProperty (Object* target, Property* prop) {  
  if(text_box_) text_box_->SetText(ReadString (target,prop));
}

void TextBoxPropertyEditor::OnTextBoxTextChanged(UINode*) {
  PropertyEditor::WriteProperty();
}

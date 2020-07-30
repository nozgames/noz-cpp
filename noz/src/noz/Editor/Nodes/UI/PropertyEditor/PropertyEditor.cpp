///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Nodes/UI/AnimationView.h>
#include "PropertyEditor.h"

using namespace noz;
using namespace noz::Editor;

PropertyEditor::PropertyEditor(void) {
  writing_property_ = false;
  reading_property_ = false;
}

PropertyEditor::~PropertyEditor(void) {
}

void PropertyEditor::SetTarget(Object* target, Property* target_property) {
  if(target == target_ && target_property_ == target_property) return;

  EditorApplication::PropertyChanged -= PropertyChangedEventHandler::Delegate(this, &PropertyEditor::OnPropertyChanged);

  target_ = target;
  target_property_ = target_property;

  OnSetTarget ();

  ReadProperty();

  if(target_ && target_property_) {
    EditorApplication::PropertyChanged += PropertyChangedEventHandler::Delegate(this, &PropertyEditor::OnPropertyChanged);
  }
}

void PropertyEditor::ReadProperty(void) {
  reading_property_ = true;
  if(target_ && target_property_) ReadProperty(target_,target_property_);
  reading_property_ = false;
}

void PropertyEditor::WriteProperty(void) {
  Inspector* inspector = GetAncestor<Inspector>();
  if(inspector && inspector->GetDocument()) {
    inspector->GetDocument()->SetModified(true);
  }

  WriteProperty(target_,target_property_);

  writing_property_ = true;
  PropertyChangedEventArgs args(target_property_,target_);
  EditorApplication::PropertyChanged(&args);
  ValueChanged(this);
  writing_property_ = false;
}

bool PropertyEditor::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  ReadProperty();
  return true;
}

void PropertyEditor::OnPropertyChanged(PropertyChangedEventArgs* args) {
  noz_assert(args);

  // Ensure the property modified is the one we care about  
  if(args->GetProperty() != target_property_) return;
  if(args->GetTarget() != target_) return;
  
  // Update the editor value using the property
  if(!writing_property_) ReadProperty();
}


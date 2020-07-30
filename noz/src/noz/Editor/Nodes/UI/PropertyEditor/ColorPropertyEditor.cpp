///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Actions/SetPropertyAction.h>
#include "ColorPropertyEditor.h"


using namespace noz;
using namespace noz::Editor;

ColorPropertyEditor::ColorPropertyEditor(void) {
  ignore_color_changes_ = false;
}

bool ColorPropertyEditor::OnApplyStyle (void) {
  if(!PropertyEditor::OnApplyStyle()) return false;
  if(nullptr == color_picker_) return false;

  color_picker_->ColorChanged += ValueChangedEventHandler::Delegate(this, &ColorPropertyEditor::OnColorPickerColorChanged);

  return true;
}

void ColorPropertyEditor::WriteProperty (Object* t, Property* p) {
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::ColorProperty)));
  noz_assert(EditorDocument::GetActiveDocument(t));

  if(nullptr == color_picker_) return;

  // Write directly to the property
  EditorDocument::GetActiveDocument(t)->ExecuteAction(new SetColorPropertyAction(t,p,color_picker_->GetColor()),!Input::GetMouseButton(MouseButton::Left));
}

void ColorPropertyEditor::ReadProperty (Object* t, Property* p) {  
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::ColorProperty)));

  ignore_color_changes_ = true;
  if(color_picker_) color_picker_->SetColor(((ColorProperty*)p)->Get(t));
  ignore_color_changes_ = false;
}

void ColorPropertyEditor::OnColorPickerColorChanged (UINode* sender) {
  if(ignore_color_changes_) return;
  PropertyEditor::WriteProperty();
}

void ColorPropertyEditor::Update (void) {
  PropertyEditor::Update();

  if(nullptr==color_picker_) return;
  if(nullptr==GetTarget()) return;
  if(nullptr==GetTargetProperty()) return;
  
  noz_assert(GetTargetProperty()->IsType(typeof(noz::ColorProperty)));

  // Look for a change in color..
  Color current = ((ColorProperty*)GetTargetProperty())->Get(GetTarget());
  if(current != color_picker_->GetColor()) {  
    ignore_color_changes_ = true;
    color_picker_->SetColor(current);
    ignore_color_changes_ = false;
  }
}


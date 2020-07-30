///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Editor/Nodes/UI/PropertyEditor/ControlPartPropertyEditor.h>
#include "StyleInspector.h"
#include "../AssetEditor/StyleEditor.h"

using namespace noz;
using namespace noz::Editor;


StyleInspector::StyleInspector(void) {
}

StyleInspector::~StyleInspector(void) {
}

void StyleInspector::OnSetTarget (Object* target) {
  Inspector::OnSetTarget(target);

  noz_assert(target);
  noz_assert(target->IsTypeOf(typeof(StyleEditorRootNode)));

  target_def_ = ((StyleEditorRootNode*)target)->style_def_;
}

bool StyleInspector::OnApplyStyle (void) {
  if(nullptr == control_type_editor_) return false;
  
  control_type_editor_->ValueChanged += ValueChangedEventHandler::Delegate(this, &StyleInspector::OnControlTypeChanged);

  if(target_def_) {
    control_type_editor_->SetTarget(target_def_,target_def_->GetProperty("ControlType"));    
  }

  RefreshControlParts();

  return true;
}

void StyleInspector::OnControlTypeChanged(UINode* sender) {
  if(target_def_) {
    StyleEditor::UpdateControlParts(target_def_);
    RefreshControlParts();
  }
}

void StyleInspector::RefreshControlParts (void) {
  if(nullptr==parts_container_) return;

  // Clear the parts list.
  parts_container_->RemoveAllChildren();

  // Ensure there is a valid style definition
  if(nullptr == target_def_) return;
  if(nullptr == target_def_->control_type_) return;

  // Cache the "Object" property of the control part 
  Property* object_property = typeof(Style::ControlPart)->GetProperty("Object");        
  if(nullptr == object_property) return;

  // Add each part
  for(noz_uint32 i=0,c=target_def_->parts_.size(); i<c; i++) {
    // Get the actual control part property from the control type
    ObjectPtrProperty* p = target_def_->control_type_->GetProperty<ObjectPtrProperty>(target_def_->parts_[i].property_); 
    if(nullptr==p) continue;

    // Create a property editor for the control part and use the object type of the source property as the base type
    ControlPartPropertyEditor* editor = new ControlPartPropertyEditor(p->GetObjectType());
    if(nullptr==editor) continue;

    // Associate the editor with the control part
    editor->SetTarget(&target_def_->parts_[i], object_property);

    // Add an inspector fow for the control part property editor
    InspectorRow* row = InspectorRow::type__->CreateInstance<InspectorRow>();
    row->SetText(target_def_->parts_[i].property_.ToCString());
    row->AddChild(editor);
    parts_container_->AddChild(row);
  }
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/Expander.h>
#include "NodeInspector.h"

using namespace noz;
using namespace noz::Editor;


NodeInspector::NodeInspector(void) {
}

NodeInspector::~NodeInspector(void) {
}

bool NodeInspector::FilterProperty (Property* p) const {
  noz_assert(p);
  if(p->GetName() == "Name") return true;
  if(p->GetName() == "Transform") return true;
  if(p->GetName() == "Components") return true;
  if(p->GetName() == "Children") return true;
  return Inspector::FilterProperty(p);
}

bool NodeInspector::OnApplyStyle (void) {
  if(nullptr == add_component_button_) return false;
  if(nullptr == add_component_type_picker_) return false;
  if(nullptr == name_editor_) return false;
  if(nullptr == type_text_) return false;
  if(nullptr == type_icon_) return false;
  if(nullptr == properties_expander_) return false;
  if(nullptr == properties_container_) return false;

  add_component_button_->Click += ClickEventHandler::Delegate(this, &NodeInspector::OnAddComponentButtonClicked);
  add_component_type_picker_->SetBaseType(typeof(Component));
  add_component_type_picker_->SelectionChanged += TypeSelectedEventHandler::Delegate(this, &NodeInspector::OnAddComponentTypeSelected);

  name_editor_->SetTarget(GetTarget(),GetTarget()->GetType()->GetProperty("Name"));

  type_text_->SetText(GetTarget()->GetType()->GetEditorName().ToCString());

  type_icon_->SetSprite(EditorFactory::CreateTypeIcon(GetTarget()->GetType()));

  Node* node = Cast<Node>(GetTarget());
  noz_assert(node);

  properties_expander_->SetText(node->GetType()->GetEditorName());
  properties_expander_->SetSprite(EditorFactory::CreateTypeIcon(node->GetType()));

  AddProperties(properties_container_);

  // Add the transform inspector
  Transform* transform = node->GetTransform();
  if(transform) AddInspector(transform);

  // Add all components..
  for(noz_uint32 i=0,c=node->GetComponentCount(); i<c; i++) AddInspector(node->GetComponent(i));

  return true;
}

void NodeInspector::OnAddComponentButtonClicked(UINode* sender) {
  if(nullptr == add_component_popup_) return;
  
  if(add_component_popup_->IsOpen()) {
    add_component_popup_->Close();
  } else {
    add_component_type_picker_->Clear();
    add_component_popup_->Open();
    add_component_type_picker_->SetFocus();
  }
}

void NodeInspector::OnAddComponentTypeSelected (UINode* sender, Type* type) {
  if(nullptr == type || !type->IsCastableTo(typeof(Component))) return;

  add_component_popup_->Close();

  Component* component = type->CreateInstance<Component>();
  if(component) {
    ((Node*)GetTarget())->AddComponent(component);
    AddInspector(component);
  }
}


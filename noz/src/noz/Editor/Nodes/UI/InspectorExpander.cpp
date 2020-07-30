///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include "InspectorExpander.h"

using namespace noz;
using namespace noz::Editor;


InspectorExpander::InspectorExpander(void) {
}

InspectorExpander::~InspectorExpander(void) {
}

bool ComponentExpander::OnApplyStyle (void) {
  if(!InspectorExpander::OnApplyStyle()) return false;
  if(nullptr == options_button_) return false;
  if(nullptr == remove_component_button_) return false;

  options_button_->Click += ClickEventHandler::Delegate(this, &ComponentExpander::OnOptionsButtonClicked);
  remove_component_button_->Click += ClickEventHandler::Delegate(this, &ComponentExpander::OnRemoveComponentButtonClicked);

  return true;
}

void ComponentExpander::OnRemoveComponentButtonClicked (UINode* sender) {
  popup_->Close();
  Node* node = component_->GetNode();
  delete component_;
  component_ = nullptr;
  PropertyChangedEventArgs args(node->GetProperty("Transform"), node);
  EditorApplication::PropertyChanged(&args);
}

void ComponentExpander::OnOptionsButtonClicked (UINode* sender) {
  if(popup_) popup_->Open();    
}

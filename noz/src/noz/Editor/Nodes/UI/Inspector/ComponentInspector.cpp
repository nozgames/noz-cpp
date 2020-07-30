///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/Expander.h>
#include "ComponentInspector.h"

using namespace noz;
using namespace noz::Editor;


ComponentInspector::ComponentInspector(void) {
}

ComponentInspector::~ComponentInspector(void) {
}

bool ComponentInspector::OnApplyStyle (void) {
  if(nullptr == expander_) return false;
  if(nullptr == options_button_) return false;
  if(nullptr == options_remove_component_) return false;

  noz_assert(GetTarget());

  if(expander_) {
    expander_->SetText(GetTarget()->GetType()->GetEditorName());
    expander_->SetSprite(EditorFactory::CreateTypeIcon(GetTarget()->GetType()));
  }

  if(properties_container_) AddProperties(properties_container_);

  options_button_->Click += ClickEventHandler::Delegate(this, &ComponentInspector::OnOptionsButtonClicked);
  options_remove_component_->Click += ClickEventHandler::Delegate(this, &ComponentInspector::OnOptionsRemoveComponent);

  return true;
}

void ComponentInspector::OnOptionsButtonClicked (UINode*) {
  if(nullptr==options_popup_) return;

  if(options_popup_->IsOpen()) {
    options_popup_->Close();
  } else {
    options_popup_->Open();
  }
}

void ComponentInspector::OnOptionsRemoveComponent (UINode*) {
  // Close the popup
  options_popup_->Close();

  Component* component = Cast<Component>(GetTarget());
  noz_assert(component);

  component->GetNode()->ReleaseComponent(component);

  // Delete the component
  delete component;
  
  // Destroy ourself
  Destroy();  
}

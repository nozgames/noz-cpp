///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/SceneNode.h>
#include "Scene.h"

using namespace noz;

Scene::Scene(void) {
  if(!Application::IsInitializing()) {
    root_ = new SceneNode;
    root_->scene_ = this;
  }
}

Scene::~Scene(void) {
  // If the scene root still exists destroy it.
  if(root_) {
    root_->scene_ = nullptr;
    root_->Destroy();
  }
}

Style* Scene::FindStyle (Type* control_type) const {
  Style* result = nullptr;
  for(noz_uint32 i=style_sheets_.size(); nullptr == result && i>0; i--) {
    StyleSheet* sheet = style_sheets_[i-1];
    noz_assert(sheet);
    result = sheet->FindStyle(control_type);
  }
  return result;
}

void Scene::OnDeserialized(void) {
  Asset::OnDeserialized();

  // Associate the scene to the root node
  if(root_) root_->scene_ = this;
}

void Scene::AddStyleSheet (StyleSheet* sheet) {
  style_sheets_.push_back(sheet);
}

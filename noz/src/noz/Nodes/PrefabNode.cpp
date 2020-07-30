//////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "PrefabNode.h"
#include "Prefab.h"

using namespace noz;

PrefabNode::PrefabNode(void) {
  template_ = nullptr;
}

void PrefabNode::Reload(void) {
  if(nullptr == template_) return;

  // Clear any existing private children.
  RemoveAllPrivateChildren();

  // Destroy the existing instance node.
  if(instance_) {
    instance_->Destroy();
    instance_ = nullptr;
  }

  // Clear instance objects
  template_objects_.clear();

  ApplyTemplate(template_);
}

void PrefabNode::ApplyTemplate(Prefab* t) {
  template_ = t;

  // Deserialize the instance directly into the prefab component
  Prefab::Instance instance(this);
  template_->instance_.DeserializeInto(&instance);

  if(instance_ == nullptr) {
    instance_ = new Node;
  }

  if(instance_->GetTransform()) {
    Transform* t = instance_->GetTransform();
    instance_->ReleaseComponent(t);
    SetTransform(t);
  }

  instance_->SetPrivate(true);
  AddChild(instance_);
}

Type* PrefabNode::GetType(void) const {
  if(nullptr==template_) return typeof(PrefabNode);
  return template_->GetPrefabNodeType();
}

Object* PrefabNode::GetPropertyProxyObject(noz_uint32 id) const {
  return template_objects_[id];
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Component.h"

using namespace noz;

Component::Component(void) {
  node_ = nullptr;
  enabled_ = true;
}

Component::~Component(void) {
  if(node_) node_->ReleaseComponent(this);
}

void Component::InvalidateTransform(void) {
  if(node_) node_->InvalidateTransform();
}

void Component::SetEnabled(bool e) {
  if(enabled_ == e) return;
  enabled_ = e;
  if(enabled_) {
    OnEnabled ();
  } else {
    OnDisabled ();
  }
}

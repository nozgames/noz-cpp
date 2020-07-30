//////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "SceneNode.h"

using namespace noz;

SceneNode::SceneNode(void) {
  SetName("SCENE");
  attr_ = attr_ | NodeAttributes::SceneRoot;
  attr_ = attr_ & (~NodeAttributes::AllowRename);
  scene_ = nullptr;
}

SceneNode::~SceneNode(void) {
  if(scene_) {
    scene_->root_ = nullptr;
    delete scene_;
  }
}

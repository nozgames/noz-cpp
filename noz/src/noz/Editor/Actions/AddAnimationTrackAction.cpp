///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AddAnimationTargetAction.h"
#include <noz/Editor/Nodes/UI/AnimationView.h>

using namespace noz;
using namespace noz::Editor;


void AddAnimationTargetAction::Do(void) {
  noz_assert(animation_);
  noz_assert(animation_target_);
  noz_assert(animation_target_->GetAnimation()==nullptr);

  // Add the target to the animation.
  animation_->AddTarget(animation_target_);
}

void AddAnimationTargetAction::Undo (void) {
  noz_assert(animation_);
  noz_assert(animation_target_->GetAnimation()==animation_);

  animation_->ReleaseTarget(animation_target_);
}

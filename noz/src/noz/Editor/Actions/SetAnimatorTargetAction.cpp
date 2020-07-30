///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "SetAnimatorTargetAction.h"
#include <noz/Editor/Nodes/UI/AnimationView.h>

using namespace noz;
using namespace noz::Editor;


void SetAnimatorTargetAction::Do(void) {
  if(animator_target_added_) {
    animator_->AddTarget(animator_target_);        
  } else {
    animator_target_->SetTarget(target_);
  }
}

void SetAnimatorTargetAction::Undo (void) {
  if(animator_target_added_) {
    animator_->ReleaseTarget(animator_target_);
  } else {
    animator_target_->SetTarget(undo_target_);
  }
}

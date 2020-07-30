///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include "EventKeyFrameInspector.h"

using namespace noz;
using namespace noz::Editor;


EventKeyFrameInspector::EventKeyFrameInspector(void) {
  key_frame_ = nullptr;
}

EventKeyFrameInspector::~EventKeyFrameInspector(void) {
}

void EventKeyFrameInspector::OnSetTarget (Object* target) {
  Inspector::OnSetTarget(target);

  noz_assert(target);
  noz_assert(target->IsTypeOf(typeof(EventKeyFrame)));

  key_frame_ = (EventKeyFrame*)target;
}

bool EventKeyFrameInspector::OnApplyStyle (void) {  
  if(key_frame_) {
    if(event_type_editor_) event_type_editor_->SetTarget(key_frame_, key_frame_->GetProperty("Event"));
    if(param_editor_) param_editor_->SetTarget(key_frame_, key_frame_->GetProperty("Method"));
  }

  return true;
}


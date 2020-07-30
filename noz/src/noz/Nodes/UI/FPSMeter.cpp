///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Render/TextNode.h>
#include "FPSMeter.h"

using namespace noz;


FPSMeter::FPSMeter(void) {
  update_interval_ = 1.0f;
  elapsed_ = 0;
  NOZ_FIXME();
//  EnableEvent(ComponentEvent::Update);
}

void FPSMeter::Update(void) {
  if(update_interval_>0) {
    elapsed_ += Time::GetDeltaTime();
    if(elapsed_ > update_interval_) {
      elapsed_ -= (((noz_int32)(elapsed_/update_interval_)) * update_interval_);
      UpdateText();
    }
  } else {
    UpdateText();
  }
}

  
void FPSMeter::UpdateText(void) {
  if(text_node_==nullptr) return;
  if(Time::GetAverageDeltaTime()>0) {
    text_node_->SetText(String::Format("%d",noz_int32(1.0f/Time::GetAverageDeltaTime()+0.5f)));
  } else {
    text_node_->SetText("0");
  }
}

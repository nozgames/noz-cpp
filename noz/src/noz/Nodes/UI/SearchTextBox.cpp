///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Render/EditableTextNode.h>
#include "SearchTextBox.h"
#include "Button.h"

using namespace noz;


bool SearchTextBox::OnApplyStyle (void) {
  if(!TextBox::OnApplyStyle()) return false;

  if(button_) button_->Click += ClickEventHandler::Delegate(this, &SearchTextBox::OnButtonClick);

  return true;
}

void SearchTextBox::OnButtonClick(UINode* sender) {
  if(GetText().IsEmpty()) {
    SetFocus();
  } else if(text_node_) {
    text_ = String::Empty;
    text_node_->SetText(text_);
    text_node_->SelectAll();        
    TextChanged(this);
    UpdateAnimationState();
  }   
}

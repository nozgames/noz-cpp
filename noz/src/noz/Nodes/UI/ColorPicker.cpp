///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/ColorPickerPopup.h>
#include "ColorPicker.h"
#include "TextBox.h"

using namespace noz;

ColorPicker::ColorPicker(void) {
  SetFocusable();
}

void ColorPicker::SetColor(Color color) {
  if(color_ == color) return;

  color_ = color;

  if(color_node_) color_node_->SetColor(Color(color_.r,color_.g,color_.b,(noz_byte)255));
  if(alpha_node_) alpha_node_->SetColor(color_);

  ColorChanged(this);
}

bool ColorPicker::OnApplyStyle(void) {
  if(!Control::OnApplyStyle()) return false;

  if(color_node_) color_node_->SetColor(Color(color_.r,color_.g,color_.b,(noz_byte)255));
  if(alpha_node_) alpha_node_->SetColor(color_);

  if(text_box_r_) text_box_r_->TextCommited += ValueChangedEventHandler::Delegate(this,&ColorPicker::OnTextCommittedRBGA);
  if(text_box_g_) text_box_g_->TextCommited += ValueChangedEventHandler::Delegate(this,&ColorPicker::OnTextCommittedRBGA);
  if(text_box_b_) text_box_b_->TextCommited += ValueChangedEventHandler::Delegate(this,&ColorPicker::OnTextCommittedRBGA);
  if(text_box_a_) text_box_a_->TextCommited += ValueChangedEventHandler::Delegate(this,&ColorPicker::OnTextCommittedRBGA);
  if(text_box_hex_) text_box_hex_->TextCommited += ValueChangedEventHandler::Delegate(this,&ColorPicker::OnTextCommittedHex);

  return true;
}

void ColorPicker::OnMouseDown(SystemEvent* e) {    
  noz_assert(e);
  noz_assert(e->GetEventType() == SystemEventType::MouseDown);

  // Handle non interactive state.
  if(!IsInteractive()) return;

  // We only care about left ButtonBase presses
  if(e->GetButton() != MouseButton::Left) return;

  // Mark the event as handled so it does not propegate further
  e->SetHandled();

  // Set the button as focused
  if(IsFocusable()) SetFocus();

  // Set the capture to this node to ensure all future events are routed to it.
  SetCapture();

  // Update the state
  UpdateAnimationState();
}

void ColorPicker::OnMouseUp(SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType() == SystemEventType::MouseUp);

  // Update state
  if(!HasCapture()) return;

  // Release capture
  ReleaseCapture();

  UpdateAnimationState();

  if(IsMouseOver()) {
    if(!ColorPickerPopup::IsOpen()) {
      ColorPickerPopup::Show(color_, PopupPlacement::Bottom, Vector2(0,-1), this, ColorChangedEvent::Delegate(this, &ColorPicker::OnColorPickerPopupColorChanged));
    } else {
      ColorPickerPopup::Hide();
    }

    /*
    if(popup_->IsOpen()) {
      popup_->Close();
    } else {
      text_box_r_->SetFocus();
      if(text_box_r_) text_box_r_->SetText(String::Format("%d", color_.r));
      if(text_box_g_) text_box_g_->SetText(String::Format("%d", color_.g));
      if(text_box_b_) text_box_b_->SetText(String::Format("%d", color_.b));
      if(text_box_a_) text_box_a_->SetText(String::Format("%d", color_.a));
      if(text_box_hex_) text_box_hex_->SetText(color_.ToString().ToCString()+1); 

      popup_->Open();
    }
    */
  }
}

void ColorPicker::OnColorPickerPopupColorChanged(Color color) {
  SetColor(color);
}

void ColorPicker::UpdateAnimationState(void) {
  if(HasCapture()) {
    if(IsMouseOver()) {
      SetAnimationState(UI::StatePressed);
    } else {
      SetAnimationState(UI::StateMouseOver);
    }
    return;
  }

  Control::UpdateAnimationState();
}

void ColorPicker::OnTextCommittedRBGA (UINode* sender) {
  Color c;
  c.r = text_box_r_ ? Int32::Parse(text_box_r_->GetText()) : 255;
  c.g = text_box_g_ ? Int32::Parse(text_box_g_->GetText()) : 255;
  c.b = text_box_b_ ? Int32::Parse(text_box_b_->GetText()) : 255;
  c.a = text_box_a_ ? Int32::Parse(text_box_a_->GetText()) : 255;
  if(text_box_hex_) text_box_hex_->SetText(c.ToString().ToCString()+1);
  SetColor(c);
}

void ColorPicker::OnTextCommittedHex (UINode* sender) {
  SetColor(Color::Parse(text_box_hex_->GetText()));
  if(text_box_r_) text_box_r_->SetText(String::Format("%d", color_.r));
  if(text_box_g_) text_box_g_->SetText(String::Format("%d", color_.g));
  if(text_box_b_) text_box_b_->SetText(String::Format("%d", color_.b));
  if(text_box_a_) text_box_a_->SetText(String::Format("%d", color_.a));
}

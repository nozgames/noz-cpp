///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/BinaryDeserializer.h>
#include "Control.h"

using namespace noz;


Control::Control (void) {    
  custom_style_ = false;
  styled_ = false;
  style_valid_ = false;
}

Control::~Control (void) {
}

void Control::SetStyle (Style* style) {
  if(style_==style) return;

  style_ = style;
  styled_ = false;
  style_valid_ = false;
  custom_style_ = style != nullptr;

  OnStyleChanged ();      

  InvalidateTransform();

  // If the control is not an orphan then apply the new template
  if(!IsOrphan()) ApplyStyle ();
}

void Control::ApplyStyle (void) {
  Type* control_type = GetType();
  Style* style = style_;

  // If there is no style set directly on the control then scan up through
  // the heirarchy for parent controls and check their styles..
  if(nullptr == style) {
    bool done = false;

    // Look for a style in the style sheet.
    if(style_sheet_) style = style_sheet_->FindStyle(control_type);

    // Check the ancestor controls
    for(Control* p=GetAncestor<Control>(); nullptr == style && p; p=p->GetAncestor<Control>()) {
      // If the control does not have its own style sheet then skip it
      if(nullptr == p->style_sheet_) continue;

      // Find the style that matches best.
      style = p->style_sheet_->FindStyle(control_type);
    }

    // Check scene as well if not done.
    if(nullptr == style) {
      Scene* scene = GetScene();
      if(scene) style = scene->FindStyle(control_type);
    }

    // If no style was found use the default style.
    if(nullptr == style) {
      String meta = control_type->GetMeta("DefaultStyle");
      if(!meta.IsEmpty()) style = AssetManager::LoadAsset<Style>(Guid::Parse(meta));
    }
  }

  // If already styled give derived classes a chance to prepare for style change
  if(styled_) OnStyleChanged();

  // Remove any old style nodes.
  RemoveAllPrivateChildren();
  
  // Mark control as styled 
  styled_ = true;
  style_ = style;

  // Deserialize the style instance into the control
  if(style) {
    Style::Template t(this);
    style->template_.DeserializeInto(&t);
  }

  style_valid_ = OnApplyStyle ();

  UpdateAnimationState();
}

void Control::SetAnimationState (const Name& state) {  
  // Base class..
  UINode::SetAnimationState(state);

  // Also set the animation state on all private nodes.
  for(noz_uint32 i=0,c=GetPrivateChildCount();i<c;i++) {
    Node* child = GetPrivateChild(i);
    for(noz_uint32 i=0,c=child->GetComponentCount(); i<c; i++) {
      Component* component = child->GetComponent(i);
      if(component->IsTypeOf(typeof(Animator))) ((Animator*)component)->SetState(state);
    }
  }
}

void Control::OnInteractiveChanged(void) {
  UINode::OnInteractiveChanged();

  UpdateAnimationState();
}

void Control::UpdateAnimationState (void) {
  // Handle disabled state..
  if(!IsInteractive()) {
    SetAnimationState(UI::StateDisabled);
    return;
  }
  // Mouse over..
  if(IsMouseOver()) {
    SetAnimationState(UI::StateMouseOver);
    return;
  }
  // Normal state
  SetAnimationState(UI::StateNormal);
}

void Control::OnLoseFocus(void) {
  UINode::OnLoseFocus();
  UpdateAnimationState();
}

void Control::OnGainFocus(void) {
  UINode::OnGainFocus();
  UpdateAnimationState();
}

void Control::OnMouseEnter(void) {
  UINode::OnMouseEnter();
  UpdateAnimationState();
}

void Control::OnMouseLeave(void) {
  UINode::OnMouseLeave();
  UpdateAnimationState();
}


Vector2 Control::Measure (const Vector2& a) {
  // If the control is not yet styled then apply one now.
  if(!IsStyled()) ApplyStyle();

  return UINode::Measure(a);      
}

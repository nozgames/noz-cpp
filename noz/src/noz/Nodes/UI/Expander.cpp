///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

#include "Expander.h"
#include "ToggleButton.h"

using namespace noz;


Expander::Expander (void) {
  expanded_ = true;
  SetLogicalChildrenOnly();
}

Expander::~Expander (void) {
}

void Expander::OnButton(void) {
  noz_assert(expand_button_);

  SetFocus();

  SetExpanded(expand_button_->IsChecked());
}

void Expander::SetExpanded(bool expanded) {
  // Is expanded state changing?
  if(expanded == expanded_) return;

  expanded_ = expanded;
  
  // Set new state and update the expanded states
  if(expand_button_) expand_button_->SetChecked(expanded_);

  UpdateExpandedState();
  UpdateAnimationState();
}

void Expander::UpdateAnimationState (void) {
  ContentControl::UpdateAnimationState();

  // Set expanded animation state
  SetAnimationState(expanded_?UI::StateExpanded:UI::StateCollapsed);
}

void Expander::UpdateExpandedState(void) {
  if(content_container_) content_container_->SetVisibility(expanded_ ? Visibility::Visible : Visibility::Collapsed);
}

bool Expander::OnApplyStyle (void) {
  if(!ContentControl::OnApplyStyle()) return false;
  if(nullptr == content_container_) return false;

  // Reparent all items
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) content_container_->AddChild(GetLogicalChild(i));

  if(expand_button_) {
    expand_button_->SetChecked(expanded_);
    expand_button_->Checked += CheckedEventHandler::Delegate(this, &Expander::OnButton);
    expand_button_->Unchecked += CheckedEventHandler::Delegate(this, &Expander::OnButton);
  }  

  UpdateExpandedState ( );

  return true;
}

void Expander::OnStyleChanged(void) {
  ContentControl::OnStyleChanged();

  // Orphan our logical children from their visual parent
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) GetLogicalChild(i)->Orphan(false);

  content_container_ = nullptr;
}

void Expander::OnChildAdded (Node* child) {
  Control::OnChildAdded(child);

  if(content_container_) content_container_->AddChild(child);
}

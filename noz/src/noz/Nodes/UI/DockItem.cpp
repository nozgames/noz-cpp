///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "DockItem.h"
#include "DockTabItem.h"
#include "DockManager.h"

using namespace noz;


DockItem::DockItem(void) {
  SetLogicalChildrenOnly();
  manager_ = nullptr;

  if(!Application::IsInitializing()) {
    tab_ = new DockTabItem;
    content_container_ = new Node;
    tab_->AddChild(content_container_);
  }
}

void DockItem::Hide (void) {
  if(GetVisibility()==Visibility::Collapsed) return;
  SetVisibility(Visibility::Collapsed);
  if(pane_) pane_->InvalidateDock();
}

void DockItem::Show (void) {
  if(GetVisibility()==Visibility::Visible) return;
  SetVisibility(Visibility::Visible);
  if(pane_) pane_->InvalidateDock();
}

void DockItem::Select (void) {
  if(nullptr == manager_) return;
  //manager_->Select(this);
}

void DockItem::SetText(const char* text) {
  if(text_.Equals(text)) return;
  text_ = text;
  if(tab_) tab_->SetText(text);
}

void DockItem::SetSprite(Sprite* sprite) {
  if(sprite_==sprite) return;
  sprite_ = sprite;
  if(tab_) tab_->SetSprite(sprite);
}

void DockItem::OnChildAdded (Node* child) {
  UINode::OnChildAdded(child);

  content_container_->AddChild(child);
}

void DockItem::Update(void) {
  UINode::Update();

/*  
  if(!content_container_->IsMouseOver()) return;
  if(!Input::GetMouseButtonDown(MouseButton::Left) && !Input::GetMouseButtonDown(MouseButton::Right)) return;

  SetFocus();

  Console::WriteLine("%d:  Focus set to %s", Application::GetFrameNumber(), GetType()->GetName().ToCString());
*/
}


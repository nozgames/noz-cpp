///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include "MessageBox.h"

using namespace noz;

/*
MessageBox::MessageBox (void) {
}
*/

MessageBoxResult MessageBox::Show (Window* parent, const char* text, const char* caption, MessageBoxButton button, MessageBoxImage image) {
  Window* window = new Window(WindowAttributes::Default,parent);

  MessageBox* box = new MessageBox;
  box->text_ = text;
  box->button_ = button;
  box->image_ = image;

  Vector2 size = box->Measure(parent->GetScreenSize());
  Rect r;
  r.x = parent->GetScreenRect().w * 0.5f - size.x * 0.5f;
  r.y = parent->GetScreenRect().h * 0.5f - size.y * 0.5f;
  r.w = size.x;
  r.h = size.y;

  Scene* scene = new Scene;
  scene->GetRootNode()->AddChild(box);
  window->SetTitle(caption);
  window->GetRootNode()->AddChild(scene->GetRootNode());

  window->MoveTo(r,parent);
  window->SetSize(size);

  window->Show();
  Application::RunModal(window);

  MessageBoxResult result = box->result_;

  delete window;

  return result;
}


bool MessageBox::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == message_text_) return false;

  bool ok = false;
  bool cancel = false;
  bool yes = false;
  bool no = false;

  switch(button_) {
    case MessageBoxButton::Ok: ok = true; break;
    case MessageBoxButton::OkCancel: ok = cancel = true; break;
    case MessageBoxButton::YesNo: yes = no = true; break;
    case MessageBoxButton::YesNoCancel: yes = no = cancel = true; break;
  }

  if(ok_button_) {if(ok) ok_button_->Click += ClickEventHandler::Delegate(this, &MessageBox::OnOkButtonClicked); else ok_button_->SetVisibility(Visibility::Collapsed);}
  if(no_button_) {if(no) no_button_->Click += ClickEventHandler::Delegate(this, &MessageBox::OnNoButtonClicked); else no_button_->SetVisibility(Visibility::Collapsed);}
  if(cancel_button_) {if(cancel) cancel_button_->Click += ClickEventHandler::Delegate(this, &MessageBox::OnCancelButtonClicked); else cancel_button_->SetVisibility(Visibility::Collapsed);}
  if(yes_button_) {if(yes) yes_button_->Click += ClickEventHandler::Delegate(this, &MessageBox::OnYesButtonClicked); else yes_button_->SetVisibility(Visibility::Collapsed);}

  message_text_->SetText(text_);

  return true;
}

void MessageBox::OnYesButtonClicked (UINode* sender) {
  result_ = MessageBoxResult::Yes;
  GetWindow()->Close();
}

void MessageBox::OnNoButtonClicked (UINode* sender) {
  result_ = MessageBoxResult::No;
  GetWindow()->Close();
}

void MessageBox::OnCancelButtonClicked (UINode* sender) {
  result_ = MessageBoxResult::Cancel;
  GetWindow()->Close();
}

void MessageBox::OnOkButtonClicked (UINode* sender) {
  result_ = MessageBoxResult::Ok;
  GetWindow()->Close();
}

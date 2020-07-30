///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/ListViewItem.h>
#include "DeployDialog.h"

using namespace noz;
using namespace noz::Editor;

DeployDialog::DeployDialog(void) {
}

DeployDialog::~DeployDialog(void) {
}

bool DeployDialog::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == platforms_) return false;
  if(nullptr == ok_button_) return false;
  if(nullptr == cancel_button_) return false;

  ListViewItem* lvitem = new ListViewItem;
  lvitem->SetText("Windows");
  lvitem->SetSprite(AssetManager::LoadAsset<Sprite>(Guid::Parse("{AAB7400D-FA3C-4C98-9452-E7F3E41C1A42}")));
  platforms_->AddChild(lvitem);

  lvitem = new ListViewItem;
  lvitem->SetText("IOS");
  lvitem->SetSprite(AssetManager::LoadAsset<Sprite>(Guid::Parse("{0C6B9DE9-A1C0-4BEC-90CF-7DF57308BA1B}")));
  platforms_->AddChild(lvitem);

  lvitem = new ListViewItem;
  lvitem->SetText("OSX");
  lvitem->SetSprite(AssetManager::LoadAsset<Sprite>(Guid::Parse("{B5795EAD-3B7C-4BA2-A3A6-D9A5FCFCB250}")));
  platforms_->AddChild(lvitem);

  lvitem = new ListViewItem;
  lvitem->SetText("Android");
  lvitem->SetSprite(AssetManager::LoadAsset<Sprite>(Guid::Parse("{FF10E3E5-6CFE-430B-AF0F-AB257FB03EDE}")));
  platforms_->AddChild(lvitem);

  ok_button_->Click += ClickEventHandler::Delegate(this, &DeployDialog::OnOk);
  cancel_button_->Click += ClickEventHandler::Delegate(this, &DeployDialog::OnCancel);

  return true;
}

void DeployDialog::OnOk (UINode*) {
  if(nullptr==platforms_->GetSelectedItem()) return;

  platform_ = Makefile::StringToPlatformType(platforms_->GetSelectedItem()->GetText().ToCString());
  if(platform_ == Makefile::PlatformType::Unknown) return;

  EndDialog(DialogBoxResult::Ok);
}

void DeployDialog::OnCancel (UINode*) {
  EndDialog(DialogBoxResult::Cancel);
}


DeployProgressDialog::DeployProgressDialog(Process* process) {
  process_ = process;
}

DeployProgressDialog::~DeployProgressDialog(void) {
  delete process_;
}

void DeployProgressDialog::Update (void) {
  DialogBox::Update();

  if(nullptr == process_ || process_->HasExited()) {
    EndDialog(DialogBoxResult::Ok);
  }
}

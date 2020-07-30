///////////////////////////////////////////////////////////////////////////////
// FarmerZ
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <FarmerZ.pch.h>
#include <noz/Nodes/Prefab.h>
#include "GameManager.h"

using namespace FarmerZ;

GameManager* GameManager::this_ = nullptr;

GameManager::GameManager (void) {
  this_ = this;
}

GameManager::~GameManager (void) {
  this_ = nullptr;
}

void GameManager::OnStart (void) {  
  if(play_button_) play_button_->Click += ClickEventHandler::Delegate(this, &GameManager::OnPlayButton);
}

void GameManager::OnUpdate (void) {  
}

void GameManager::TransitionIn (void) {
}

void GameManager::TransitionOut (void) {
}

void GameManager::OnPlayButton(UINode* sender) {
  if(play_button_sound_) play_button_sound_->Play();
  GetNode()->SetAnimationState("MainMenuHide");

//  Scene* scene = AssetManager::LoadAsset<Scene>(Guid::Parse("{0FA909DE-E236-4043-A521-A9B3DE9B79D3}"));
//  if(nullptr == scene) return;
//  GetNode()->GetWindow()->GetRootNode()->AddChild(scene->GetRootNode());
}

void GameManager::StateResetGame (void) {
  ResetBoard ( );
  GetNode()->SetAnimationState("StartGame");
}

void GameManager::StateStartGame (void) {
}

void GameManager::ResetBoard (void) {
  if(nullptr == board_) return;
  if(animals_.empty()) return;

  board_->RemoveAllChildren();

  for(noz_uint32 x=0; x<6; x++) {
    for(noz_uint32 y=0; y<8; y++) {
      PrefabNode* node = animals_[Math::Random(0,animals_.size()-1)]->Instantiate();
      node->GetTransform()->SetLocalPosition(Vector2(50.0f + 100.0f * x, 50.0f + 100.0f * y));
      board_->AddChild(node);
    }
  }    
}

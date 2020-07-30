///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Actions/SetPropertyAction.h>
#include <noz/Editor/Nodes/UI/AnimationView.h>
#include "SpritePropertyEditor.h"


using namespace noz;
using namespace noz::Editor;

SpritePropertyEditor::SpritePropertyEditor(void) {  
}

bool SpritePropertyEditor::OnApplyStyle (void) {  
  if(nullptr == button_) return false;
  if(nullptr == asset_picker_) return false;

  button_->Click += ClickEventHandler::Delegate(this,&SpritePropertyEditor::OnButtonClick);
  button_->SetDragDropTarget(true);
  button_->DragDrop += DragDropEventHandler::Delegate(this,&SpritePropertyEditor::OnButtonDragDrop);
 
  asset_picker_->SelectionChanged += AssetSelectedEventHandler::Delegate(this,&SpritePropertyEditor::OnAssetSelected);

  SetSprite(nullptr);

  return PropertyEditor::OnApplyStyle();
}

void SpritePropertyEditor::WriteProperty (Object* t, Property* p) {
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::ObjectPtrProperty)));
  noz_assert(((ObjectPtrProperty*)p)->GetObjectType() == typeof(Sprite));

  if(nullptr==sprite_node_) return;

  EditorDocument::GetActiveDocument(this)->ExecuteAction(new SetObjectPtrPropertyAction(t,p,sprite_node_->GetSprite()));
}

void SpritePropertyEditor::ReadProperty (Object* t, Property* p) {  
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::ObjectPtrProperty)));
  noz_assert(((ObjectPtrProperty*)p)->GetObjectType() == typeof(Sprite));

  SetSprite((Sprite*)((ObjectPtrProperty*)p)->Get(t));
}

void SpritePropertyEditor::OnButtonClick (UINode* sender) {
  if(popup_) {
    // Clear the filter text before opening the popup again
    asset_picker_->Clear();
    popup_->Open();
  }
}

void SpritePropertyEditor::OnAssetSelected (UINode* sender, Asset* asset) {
  SetSprite((Sprite*)asset);

  PropertyEditor::WriteProperty();
}

void SpritePropertyEditor::OnButtonDragDrop (UINode* sender, DragDropEventArgs* args) {
  if(nullptr==args->GetObject()) return;

  switch(args->GetEventType()) {
    case DragDropEventType::Over:
    case DragDropEventType::Enter: {
      AssetFile* file = args->GetObject<AssetFile>();
      if(file) {
        if(file->GetAssetType()->IsCastableTo(typeof(Sprite))) {
          args->SetEffects(DragDropEffects::Copy);
        }
      }
      break;
    }

    case DragDropEventType::Drop: {
      AssetFile* file = args->GetObject<AssetFile>();
      if(file) {
        SetSprite(AssetManager::LoadAsset<Sprite>(file->GetGuid()));
        PropertyEditor::WriteProperty();      
      }
      args->SetEffects(DragDropEffects::Copy);
      break;
    }
  }
}

void SpritePropertyEditor::SetSprite (Sprite* sprite) {
  if(nullptr==sprite_node_) return;
  sprite_node_->SetSprite(sprite);
  UpdateAnimationState();
}

void SpritePropertyEditor::UpdateAnimationState(void) {
  PropertyEditor::UpdateAnimationState();

  if(nullptr==sprite_node_ || nullptr==sprite_node_->GetSprite()) {
    SetAnimationState(UI::StateEmpty);
  } else {
    SetAnimationState(UI::StateNotEmpty);
  }
}

void SpritePropertyEditor::Update(void) {
  PropertyEditor::Update();

  if(nullptr==sprite_node_) return;
  if(nullptr==GetTarget()) return;
  if(nullptr==GetTargetProperty()) return;
  
  noz_assert(GetTargetProperty()->IsType(typeof(noz::ObjectPtrProperty)));

  // Look for a change
  Sprite* sprite = Cast<Sprite>(((ObjectPtrProperty*)GetTargetProperty())->Get(GetTarget()));
  if(sprite_node_->GetSprite() != sprite) {  
    SetSprite(sprite);
  }
}
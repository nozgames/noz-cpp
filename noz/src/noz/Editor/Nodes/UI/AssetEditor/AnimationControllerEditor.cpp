///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Serialization/JsonSerializer.h>
#include "AnimationControllerEditor.h"

using namespace noz;
using namespace noz::Editor;


AnimationControllerEditor::AnimationControllerEditor(void) {
}  

AnimationControllerEditor::~AnimationControllerEditor(void) {
}

bool AnimationControllerEditor::OnApplyStyle (void) {
  if(!AssetEditor::OnApplyStyle()) return false;
  if(nullptr == tree_view_) return false;

  tree_view_->SelectionChanged += SelectionChangedEventHandler::Delegate(this, &AnimationControllerEditor::OnTreeViewSelectionChanged);
  if(add_layer_button_) add_layer_button_->Click += ClickEventHandler::Delegate(this, &AnimationControllerEditor::OnAddLayerButton);
  if(add_state_button_) add_state_button_->Click += ClickEventHandler::Delegate(this, &AnimationControllerEditor::OnAddStateButton);

  Refresh();

  return true;
}

bool AnimationControllerEditor::Load (AssetFile* file) {
  controller_ = AssetManager::LoadAsset<AnimationController>(file->GetGuid());
  inspector_ = EditorFactory::CreateInspector(controller_);
  SetInspector(inspector_);
  Refresh();
  return true;
}

void AnimationControllerEditor::Refresh (void) {
  if(tree_view_ == nullptr) return;

  // Reset the tree view
  tree_view_->RemoveAllChildren();

  // If no controller leave it empty..
  if(nullptr == controller_) return;

  for(noz_uint32 i=0,c=controller_->GetLayerCount(); i<c; i++) {
    AnimationLayer* layer = controller_->GetLayer(i);

    AnimationControllerEditorLayer* layer_item = new AnimationControllerEditorLayer(layer);
    layer_item->SetText(layer->GetName().IsEmpty()?"[Unnamed]":layer->GetName().ToCString());
    tree_view_->AddChild(layer_item);

    for(noz_uint32 ii=0,cc=layer->GetStateCount(); ii<cc; ii++) {
      AnimationState* state = layer->GetState(ii);      

      AnimationControllerEditorState* state_item = new AnimationControllerEditorState(state);
      state_item->SetText(state->GetName());
      layer_item->AddChild(state_item);
    }
  }

  tree_view_->ExpandAll();
}

void AnimationControllerEditor::OnAddLayerButton (UINode* sender) {
  noz_assert(controller_);

  AnimationLayer* layer = new AnimationLayer;
  controller_->AddLayer(layer);

  AnimationControllerEditorLayer* layer_item = new AnimationControllerEditorLayer(layer);
  layer_item->SetText(layer->GetName().IsEmpty()?"[Unnamed]":layer->GetName().ToCString());
  tree_view_->AddChild(layer_item);
}

void AnimationControllerEditor::OnAddStateButton (UINode* sender) {
  noz_assert(controller_);
  
  AnimationLayer* layer = nullptr;

  TreeViewItem* item = tree_view_->GetSelectedItem();
  if(item==nullptr) return;


  AnimationControllerEditorLayer* layer_item = nullptr;
  if(item->IsTypeOf(typeof(AnimationControllerEditorLayer))) {
    layer_item = ((AnimationControllerEditorLayer*)item);
    layer = layer_item->layer_;
  } else if(item->IsTypeOf(typeof(AnimationControllerEditorState))) {
    layer_item = ((AnimationControllerEditorLayer*)item->GetParentItem());
    layer = layer_item->layer_;
  }

  noz_assert(layer);

  // Add the new state to the layer
  AnimationState* state = new AnimationState;
  state->SetName("Unnammed");
  layer->AddState(state);

  // Add the new item to the layer
  AnimationControllerEditorState* state_item = new AnimationControllerEditorState(state);
  state_item->SetText(state->GetName());
  layer_item->AddChild(state_item);
  layer_item->ExpandAll();
}

void AnimationControllerEditor::OnTreeViewSelectionChanged (UINode* sender) {
  if(inspector_) {
    TreeViewItem* item = tree_view_->GetSelectedItem();
    if(item->IsTypeOf(typeof(AnimationControllerEditorLayer))) {
      SetInspector(EditorFactory::CreateInspector(((AnimationControllerEditorLayer*)item)->layer_));
    } else if(item->IsTypeOf(typeof(AnimationControllerEditorState))) {
      SetInspector(EditorFactory::CreateInspector(((AnimationControllerEditorState*)item)->state_));
    }
  }
}

void AnimationControllerEditor::Save (void) {
  FileStream fs;
  if(!fs.Open(GetFile()->GetPath(),FileMode::Truncate)) {
    Console::WriteError(this, "failed to save sprite");
    return;
  }

  JsonSerializer().Serialize(controller_,&fs);
  fs.Close();

  AssetDatabase::ReloadAsset (GetFile()->GetGuid());

  SetModified(false);
}


void AnimationControllerEditorLayer::Update (void) {
  TreeViewItem::Update();
  SetText(layer_->GetName().IsEmpty() ? "[Unnamed]" : layer_->GetName().ToCString());
}

void AnimationControllerEditorState::Update (void) {
  TreeViewItem::Update();
  SetText(state_->GetName());
}


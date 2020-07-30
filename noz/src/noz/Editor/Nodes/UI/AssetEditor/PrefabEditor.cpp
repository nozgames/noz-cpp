///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Nodes/UI/Hierarchy.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Serialization/JsonDeserializer.h>
#include "PrefabEditor.h"

using namespace noz;
using namespace noz::Editor;


PrefabEditor::PrefabEditor(void) {  
  // Create a new inspector for scene editor.
  if(!Application::IsInitializing()) {
    // Create a new hierarchy for scene editor.
    if(nullptr==hierarchy_) {
      hierarchy_ = new Hierarchy;
      hierarchy_->SelectionChanged += SelectionChangedEventHandler::Delegate(this,&PrefabEditor::OnHeirarchySelectionChanged);
      SetHierarchy(hierarchy_);
    }
  }
}

PrefabEditor::~PrefabEditor(void) {
  if(root_node_) root_node_->Destroy();
  if(hierarchy_) hierarchy_->Destroy();
}

bool PrefabEditor::OnApplyStyle (void) {
  if(!AssetEditor::OnApplyStyle()) return false;
  if(nullptr == content_container_) return false;

  // Reparent the loaded style's nodes to the new root node.
  content_container_->AddChild(root_node_);

  hierarchy_->SetTarget(root_node_);  

  HierarchyItem* item = hierarchy_->FindItem(nullptr, root_node_);
  if(item) item->SetSelected(true);

  return true;
}

bool PrefabEditor::Load (AssetFile* file) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(file->GetPath(),FileMode::Open)) return nullptr;

  // Deserialize the stream into a StyleDef objestyle
  if(nullptr==JsonDeserializer().Deserialize(&fs,&prefab_def_)) {
    return nullptr;
  }

  // Close the input file
  fs.Close();

  // Remove any old root node if there is one.
  if(root_node_) root_node_->Destroy();

  // Create the new root node
  root_node_ = new PrefabEditorRootNode(&prefab_def_);

  // Add the prefab node to the root
  if(prefab_def_.node_) root_node_->AddChild(prefab_def_.node_);

  // Add the prefab root to the content container.
  if(content_container_) content_container_->AddChild(root_node_);

  // Use the root node for the base of the hierarchy  
  hierarchy_->SetTarget(root_node_);

  return true;
}

void PrefabEditor::OnHeirarchySelectionChanged (UINode* sender) {
  // Inspect the newly selected node
  NOZ_TODO("handle inspector locking here");
  SetInspector(EditorFactory::CreateInspector(hierarchy_->GetSelected()));
  
  // Find the animator that controls the selected node.  The controlling animator is 
  // either the animator on the node itself or the first animator in the nodes ancestry.
  Animator* animator = nullptr;
  for(Node* node = hierarchy_->GetSelected(); !animator && node && node!=root_node_; node = node->GetLogicalParent()) {
    animator = node->GetComponent<Animator>();
  }

  // Set the new animation view source to the controlling animator
  SetAnimationViewSource(animator);
}

void PrefabEditor::Save (void) {
  FileStream fs;
  if(!fs.Open(GetFile()->GetPath(),FileMode::Truncate)) {
    Console::WriteError(this, "failed to save style");
    return;
  }

  prefab_def_.node_ = nullptr;
  if(root_node_->GetLogicalChildCount()) {
    prefab_def_.node_ = root_node_->GetLogicalChild(0);
  }

  JsonSerializer().Serialize(&prefab_def_,&fs);
  fs.Close();

  AssetDatabase::ReloadAsset (GetFile()->GetGuid());
  SetModified(false);
}

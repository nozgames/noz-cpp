///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Nodes/UI/Hierarchy.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Serialization/JsonDeserializer.h>
#include "StyleEditor.h"

using namespace noz;
using namespace noz::Editor;


StyleEditor::StyleEditor(void) {  
  zoom_= 1.0f;

  // Create a new inspector for scene editor.
  if(!Application::IsInitializing()) {
    // Create a new hierarchy for scene editor.
    if(nullptr==hierarchy_) {
      hierarchy_ = new Hierarchy;
      hierarchy_->SelectionChanged += SelectionChangedEventHandler::Delegate(this,&StyleEditor::OnHeirarchySelectionChanged);
      SetHierarchy(hierarchy_);
    }
  }
}

StyleEditor::~StyleEditor(void) {
  if(root_node_) root_node_->Destroy();
  if(hierarchy_) hierarchy_->Destroy();
}

bool StyleEditor::OnApplyStyle (void) {
  if(!AssetEditor::OnApplyStyle()) return false;
  if(nullptr == content_container_) return false;

  // Reparent the loaded style's nodes to the new root node.
  content_container_->AddChild(root_node_);

  hierarchy_->SetTarget(root_node_);  

  HierarchyItem* item = hierarchy_->FindItem(nullptr, root_node_);
  if(item) item->SetSelected(true);

  return true;
}

void StyleEditor::OnStyleChanged (void) {
  AssetEditor::OnStyleChanged();

  if(root_node_) root_node_->Orphan();

  content_container_ = nullptr;
}

void StyleEditor::UpdateControlParts (Style::Def* def) {
  // Make a copy of existing parts..  
  std::vector<Style::ControlPart> existing_parts = def->parts_;
  
  // Clear the parts list and populate with the actual control parts 
  def->parts_.clear();
  for(Type* t=def->control_type_; t; t=t->GetBase()) {
    for(auto itp=t->GetProperties().begin(); itp!=t->GetProperties().end(); itp++) {
      Property* p = *itp;
      if(!p->IsControlPart()) continue;
      Style::ControlPart part;
      part.property_ = p->GetName();
      def->parts_.push_back(part);
    }
  }

  // Now fill in the values we already know
  for(noz_uint32 i=0,c=existing_parts.size(); i<c; i++) {
    Style::ControlPart& existing = existing_parts[i];
    for(noz_uint32 ii=0,cc=def->parts_.size(); ii<cc; ii++) {
      if(def->parts_[ii].property_ == existing.property_) {
        def->parts_[ii].object_ = existing.object_;
        break;
      }
    }
  }
}

bool StyleEditor::Load (AssetFile* file) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(file->GetPath(),FileMode::Open)) return nullptr;

  // Deserialize the stream into a StyleDef objestyle
  if(nullptr==JsonDeserializer().Deserialize(&fs,&style_def_)) {
    return nullptr;
  }

  // Close the input file
  fs.Close();

  UpdateControlParts(&style_def_);

  // Remove any old root node if there is one.
  if(root_node_) root_node_->Destroy();

  // Create the new root node
  root_node_ = new StyleEditorRootNode(&style_def_);

  // Add the style nodes to the root
  for(noz_uint32 i=0,c=style_def_.nodes_.size(); i<c; i++) root_node_->AddChild(style_def_.nodes_[i]);

  // Add the style root to the content container.
  if(content_container_) content_container_->AddChild(root_node_);

  // Use the root node for the base of the hierarchy  
  hierarchy_->SetTarget(root_node_);

  return true;
}

void StyleEditor::OnHeirarchySelectionChanged (UINode* sender) {
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

void StyleEditor::Save (void) {
  FileStream fs;
  if(!fs.Open(GetFile()->GetPath(),FileMode::Truncate)) {
    Console::WriteError(this, "failed to save style");
    return;
  }

  style_def_.nodes_.clear();
  for(noz_uint32 i=0,c=root_node_->GetChildCount(); i<c; i++) {
    style_def_.nodes_.push_back(root_node_->GetChild(i));
  }

  JsonSerializer().Serialize(&style_def_,&fs);
  fs.Close();

  AssetDatabase::ReloadAsset (GetFile()->GetGuid());
  SetModified(false);
}

void StyleEditor::OnMouseDown (SystemEvent* e) {
  Node* n = Node::HitTest(root_node_, e->GetPosition());
  if(n) {
    hierarchy_->SetSelected(n);
  }
}


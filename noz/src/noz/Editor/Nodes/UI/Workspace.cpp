///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/System/Process.h>
#include <noz/Nodes/UI/DocumentManager.h>
#include <noz/Nodes/UI/Document.h>
#include <noz/Nodes/UI/DockManager.h>
#include <noz/Nodes/UI/DockItem.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Editor/Nodes/UI/AssetEditor/AssetEditor.h>
#include <noz/Editor/Nodes/UI/AnimationView.h>
#include <noz/Editor/Nodes/UI/DeployDialog.h>
#include "Workspace.h"
#include "Inspector.h"
#include "Hierarchy.h"
#include "TypePicker.h"

using namespace noz;
using namespace noz::Editor;


Workspace::Workspace(void) {
}

Workspace::~Workspace(void) {
  if(hierarchy_) hierarchy_->Orphan();
  if(inspector_) inspector_->Orphan();
}

void Workspace::EditAsset(AssetFile* file) {
  noz_assert(file);

  if(nullptr == document_manager_) return;

  for(auto it=document_manager_->GetDocuments().begin(); it!=document_manager_->GetDocuments().end(); it++) {
    noz_assert(*it);
    Document* document = *it;
    if(!document->IsTypeOf(typeof(AssetEditor))) continue;

    if(((AssetEditor*)document)->GetFile() == file) {
      document_manager_->SetActiveDocument(document);
      return;
    }
  }

  AssetEditor* editor = file->CreateEditor();
  if(nullptr == editor) {
    return;
  }

  editor->SetName(file->GetName());
  document_manager_->AddDocument(editor);
  document_manager_->SetActiveDocument(editor);
  editor->Open(file);
}

void Workspace::SetAnimationViewAnimator (Animator* animator) {
  if(animation_view_animator_ == animator) return;
  animation_view_animator_ = animator;
  if(animation_view_) animation_view_->SetSource(animator);
}

void Workspace::SetInspector (Inspector* inspector, bool own) {
  if(inspector_==inspector) return;
  if(inspector_) {
    if(inspector_owned_) {
      inspector_->Destroy();
    } else {
      inspector_->Orphan();
    }
  }

  inspector_owned_ = own;
  inspector_ = inspector;
  if(inspector_dock_item_) {
    inspector_dock_item_->RemoveAllChildren();
    if(inspector) {
      inspector_dock_item_->AddChild(inspector);    
    } else {
      inspector_dock_item_->AddChild(new Inspector);
    }
  }
}

void Workspace::SetHierarchy (Hierarchy* hierarchy) {
  if(hierarchy_==hierarchy) return;
  if(hierarchy_) hierarchy_->Orphan();
  hierarchy_ = hierarchy;
  if(hierarchy_dock_item_) {    
    if(hierarchy) {
      hierarchy_dock_item_->AddChild(hierarchy);    
      hierarchy_dock_item_->Show();
    } else {
      hierarchy_dock_item_->Hide();
    }
  }
}

Workspace* Workspace::GetWorkspace (Node *node) {
  if(nullptr==node) return nullptr;
  while(node && !node->IsTypeOf(typeof(Workspace))) node=node->GetParent();
  return (Workspace*)node;
}

void Workspace::OnStyleChanged (void) {
  Control::OnStyleChanged();

  if(hierarchy_) hierarchy_->Orphan();
  if(inspector_) inspector_->Orphan();

  hierarchy_dock_item_ = nullptr;
  inspector_dock_item_ = nullptr;
}

bool Workspace::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == hierarchy_dock_item_) return false;
  if(nullptr == inspector_dock_item_) return false;

  if(hierarchy_) {
    hierarchy_dock_item_->AddChild(hierarchy_);    
    if(hierarchy_) {
      hierarchy_dock_item_->Show();
    } else {
      hierarchy_dock_item_->Hide();
    }
  } else {
    hierarchy_dock_item_->Hide();
  }

  inspector_dock_item_->AddChild(new Inspector);

  SetFocus();

  return true;
}

void Workspace::OnPreviewKeyDown (SystemEvent* e) {
  Control::OnPreviewKeyDown(e);  

  switch(e->GetKeyCode()) {
    case Keys::S: {
      if(e->IsControl()) {
        if(document_manager_) {
          Document* doc = document_manager_->GetActiveDocument();
          if(doc) {
            doc->Save();
            e->SetHandled();
          }
        }

        if(animation_view_) animation_view_->Save();
      }
      break;
    }

    case Keys::F1: {
      DeployDialog* dd = new DeployDialog;
      dd->SetTitle("Deploy");
      if(DialogBoxResult::Ok == dd->DoModal(GetWindow())) {        
        Process* process = Process::Start(Environment::GetExecutablePath().ToCString(), 
          String::Format("-deploy -platform=%s -target=e:\\deploy\\%s", Makefile::PlatformTypeToString(dd->GetPlatform()).ToCString(), Makefile::PlatformTypeToString(dd->GetPlatform()).ToCString()).ToCString());
        if(process) {
          DeployProgressDialog* dpd = new DeployProgressDialog(process);
          dpd->SetTitle("Deploy");
          dpd->DoModal(GetWindow());
          delete dpd;
        }
      }
      delete dd;
      break;
    }      

    case Keys::Z: {
      if(e->IsControl()) {
        EditorDocument* d = GetActiveDocument();
        if(d) {
          d->Undo();
        }

        e->SetHandled();
        break;
      }
    }

    case Keys::Y: {
      if(e->IsControl()) {
        EditorDocument* d = GetActiveDocument();
        if(d) {
          d->Redo();
        }

        e->SetHandled();
        break;
      }
    }
  }
}

void Workspace::OnKeyDown (SystemEvent* e) {
  Control::OnKeyDown(e);  
}

EditorDocument* Workspace::GetActiveDocument (void) const {
  if(nullptr == document_manager_) return nullptr;
  return Cast<EditorDocument>(document_manager_->GetActiveDocument());
}

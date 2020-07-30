///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Render/TextNode.h>

#include "DocumentManager.h"
#include "DocumentTabItem.h"
#include "Document.h"
#include "TabItem.h"

using namespace noz;

DocumentManager::DocumentManager(void) {
}

DocumentManager::~DocumentManager(void) {
}

bool DocumentManager::OnApplyStyle(void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == tab_control_) return false;

  Document* old_selected = selected_;

  SetActiveDocument(nullptr);

  for(noz_uint32 i=0,c=documents_.size(); i<c; i++) {
    Document* d = documents_[i];
    noz_assert(d);
    noz_assert(d->tab_==nullptr);
      
    AddDocumentTab(d);
  }

  if(old_selected) SetActiveDocument(old_selected);

  tab_control_->SelectionChanged += SelectionChangedEventHandler::Delegate(this,&DocumentManager::OnTabSelectionChanged);

  return true;
}

void DocumentManager::OnStyleChanged (void) {
  Control::OnStyleChanged();

  // Unlink all of the documents..
  if(tab_control_) {
    for(noz_uint32 i=0,c=documents_.size(); i<c; i++) {
      Document* d = documents_[i];
      noz_assert(d);

      d->Orphan();

      // Remove the old tab..
      if(d->tab_) {
        d->tab_->Destroy();
        d->tab_ = nullptr;
      }
    }
    tab_control_ = nullptr;
  }
}

void DocumentManager::SetActiveDocument (Document* document) {
  if(document && document->manager_ != this) return;

  if(selected_) selected_->OnDeactivate();

  // Set the new document as selected
  selected_ = document;

  // Select the tab associated with the document
  if(document) {
    document->tab_->Select();

    // Set focus to the tab control
    if(tab_control_) tab_control_->SetFocus();

    document->OnActivate();
  }
}

void DocumentManager::AddDocument (Document* document) {
  if(document->manager_) return;

  // Add the document to the list
  documents_.push_back(document);

  // Link the document to the manager
  document->manager_ = this;

  // If styled add the document to the list.
  if(tab_control_) AddDocumentTab(document);    
}

void DocumentManager::AddDocumentTab (Document* d) {  
  noz_assert(d);
  noz_assert(tab_control_);

  // Create a tab for the document and link it to the document
  d->tab_ = new DocumentTabItem;
  d->tab_->SetDocument(d);

  // Add the tab to the tab control
  tab_control_->AddChild(d->tab_);
}

void DocumentManager::OnMouseDown (SystemEvent* e) {
  Control::OnMouseDown (e);
     
  e->SetHandled();
  tab_control_->SetFocus();
}

void DocumentManager::CloseDocument (Document* document) {
  if(document->manager_ != this) return;

  // Remove the document from the documents list
  noz_uint32 di = 0;
  for(noz_uint32 c=documents_.size(); di<c; di++) {
    if(documents_[di] == document) {
      documents_.erase(documents_.begin()+di);
      break;
    }
  }

  noz_int32 select = Math::Min(((noz_int32)documents_.size())-1,(noz_int32)di);
  if(select>=0) {
    SetActiveDocument (documents_[select]);
  } else {
    SetActiveDocument (nullptr);
  }

  // Remove the item from the tab.
  if(tab_control_) tab_control_->RemoveChildAt(document->tab_->GetIndex());
}

void DocumentManager::OnTabSelectionChanged (UINode* sender) {
  if(selected_) selected_->OnDeactivate();

  if(tab_control_->GetSelected()) {
    selected_ = ((DocumentTabItem*)tab_control_->GetSelected())->GetDocument();
    selected_->OnActivate();
  } else {
    selected_ = nullptr;
  }
}

void DocumentManager::OnKeyDown (SystemEvent* e) {
  Control::OnKeyDown(e);

#if defined(NOZ_WINDOWS)
  if(e->IsControl() && e->GetKeyCode() == Keys::F4 && GetActiveDocument()) {
    CloseDocument(GetActiveDocument());
  }
#endif
}

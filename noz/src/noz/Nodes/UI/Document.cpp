///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Render/TextNode.h>
#include "Document.h"
#include "DocumentManager.h"
#include "DocumentTabItem.h"

using namespace noz;


Document::Document(void) {
  manager_ = nullptr;
  modified_ = false;
}

Document::~Document(void) {
}

bool Document::IsActive (void) const {
  return (manager_ && manager_->selected_ == this);
}

void Document::SetModified(bool modified) {
  if(modified==modified_) return;
   
  modified_ = modified;

  UpdateTitle();
}

void Document::UpdateTitle(void) {
  if(manager_ == nullptr) return;
  if(tab_ == nullptr) return;

  if(modified_) {
    tab_->SetText(String::Format("%s*",name_.ToCString()));
  } else {  
    tab_->SetText(name_.ToCString());
  }
}

void Document::Close (void) {
  if(manager_) manager_->CloseDocument(this);
}


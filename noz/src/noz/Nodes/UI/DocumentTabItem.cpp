///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Render/TextNode.h>
#include "Button.h"
#include "Document.h"
#include "DocumentTabItem.h"

using namespace noz;


DocumentTabItem::DocumentTabItem(void) {
}

DocumentTabItem::~DocumentTabItem(void) {
}

bool DocumentTabItem::OnApplyStyle (void) {
  if(!TabItem::OnApplyStyle()) return false;

  if(close_button_) close_button_->Click += ClickEventHandler::Delegate(this,&DocumentTabItem::OnCloseButton);

  if(document_) SetText(document_->GetName());

  return true;
}

void DocumentTabItem::SetDocument (Document* document) {
  if(document_==document) return;
  document_ = document;
  if(document_) {
    AddChild(document);
    SetText(document->GetName());
  }
}

void DocumentTabItem::OnCloseButton (UINode* sender) {
  document_->Close();
}

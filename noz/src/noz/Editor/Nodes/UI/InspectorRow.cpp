///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "InspectorRow.h"

using namespace noz;
using namespace noz::Editor;


InspectorRow::InspectorRow(void) {
  SetLogicalChildrenOnly();
}

InspectorRow::~InspectorRow(void) {
}

bool InspectorRow::OnApplyStyle (void) {
  if(!ContentControl::OnApplyStyle()) return false;
  if(nullptr == content_container_) return false;

  // Reparent all items
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) content_container_->AddChild(GetLogicalChild(i));

  return true;
}

void InspectorRow::OnStyleChanged (void) {
  ContentControl::OnStyleChanged();

  // Orphan our logical children from their visual parent
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) GetLogicalChild(i)->Orphan(false);

  content_container_ = nullptr;
}


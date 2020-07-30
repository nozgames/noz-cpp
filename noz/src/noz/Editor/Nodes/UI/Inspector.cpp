///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Nodes/UI/PropertyEditor/PropertyEditor.h>
#include "Inspector.h"
#include "InspectorRow.h"

using namespace noz;
using namespace noz::Editor;


Inspector::Inspector(void) {
}

Inspector::~Inspector(void) {
}

bool Inspector::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == content_container_) return false;

  AddProperties (content_container_);

  return true;
}

void Inspector::SetTarget (Object* t) {
  if(target_ == t) return;
  target_ = t;
  OnSetTarget(target_);
}

void Inspector::SetDocument(EditorDocument* d) {
  if(document_ == d) return;
  document_ = d;
}

void Inspector::OnSetTarget(Object* t) {
}

void Inspector::AddProperties(Node* container) {
  if(nullptr == target_) return;
  for(Type* type=target_->GetType();type;type=type->GetBase()) {
    for(auto it=type->GetProperties().begin(); it!=type->GetProperties().end(); it++) {
      Property* prop = *it;
      if(FilterProperty(prop)) continue;
      
      PropertyEditor* editor = EditorFactory::CreatePropertyEditor(prop);
      if(editor) {
        editor->SetTarget (target_, prop);

        if(!editor->IsExpander()) {
          InspectorRow* row = InspectorRow::type__->CreateInstance<InspectorRow>();
          row->SetText(prop->GetName().ToCString());
          row->AddChild(editor);
          container->AddChild(row);
        } else {
          container->AddChild(editor);
        }
      }
    }
  }
}

void Inspector::AddInspector (Object* target) {
  if(nullptr == content_container_) return;

  Inspector* inspector = EditorFactory::CreateInspector(target);
  if(nullptr == inspector) return;
  content_container_->AddChild(inspector);
}

bool Inspector::FilterProperty(Property* p) const {
  noz_assert(p);
  if(p->IsPrivate()) return true;
  if(p->IsWriteOnly()) return true;
  return !Boolean::Parse(p->GetMeta("EditorVisible","true"));
}

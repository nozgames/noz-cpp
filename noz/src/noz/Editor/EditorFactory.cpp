///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "EditorFactory.h"
#include "Nodes/UI/PropertyEditor/PropertyEditor.h"
#include "Nodes/UI/Workspace.h"

using namespace noz;
using namespace noz::Editor;

EditorFactory* EditorFactory::this_ = nullptr;

static const Name MetaEditorProperty ("EditorProperty");
static const Name MetaEditorData ("EditorData");
static const Name MetaEditorIcon ("EditorIcon");
static const Name MetaEditorTarget ("EditorTarget");


void EditorFactory::Initialize (void) {
  if(this_) return;

  this_ = new EditorFactory;
  this_->RegisterPropertyEditors();
  this_->RegisterInspectors();
}

void EditorFactory::Uninitialize (void) {
  if(nullptr==this_) return;
  delete this_;
  this_ = nullptr;
}

PropertyEditor* EditorFactory::CreatePropertyEditor (Property* p) {
  Type* d = nullptr;
  if(p->IsType(typeof(ObjectPtrProperty))) {
    d = ((ObjectPtrProperty*)p)->GetObjectType();
  } else if(p->IsType(typeof(ObjectProperty))) {
    d = ((ObjectProperty*)p)->GetObjectType();
  } else if(p->IsType(typeof(EnumProperty))) {
    d = ((EnumProperty*)p)->GetEnumType();
  }

  RegisteredPropertyEditor* result = nullptr;
  for(noz_uint32 i=0,c=this_->property_editors_.size(); i<c; i++) {
    RegisteredPropertyEditor& pe = this_->property_editors_[i];
    if(pe.property_type_ != p->GetType()) continue;
    
    if(pe.data_type_) {
      if(d && d->IsCastableTo(pe.data_type_)) {
        result = &pe;
      }
    } else if(nullptr == result) {
      result = &pe;
    }    
  }

  if(nullptr == result) {
    Console::WriteLine("error: no property editor defined for Property type '%s'", p->GetType()->GetQualifiedName().ToCString());
    return nullptr;
  }

  PropertyEditor* editor = result->editor_type_->CreateInstance<PropertyEditor>();
  if(nullptr == editor) {
    Console::WriteLine("error: failed to create PropertyEditor of type '%s' for Property type '%s'", result->editor_type_->GetQualifiedName().ToCString(), p->GetType()->GetQualifiedName().ToCString());
    return nullptr;
  }

  return editor;
}

void EditorFactory::RegisterPropertyEditors(void) {
  // Iterate over all known types and find any that are a subclass of AssetEditor
  const std::vector<Type*>& types = Type::GetTypes();
  for(auto it=types.begin(); it!=types.end(); it++) {
    Type* t = *it;
    noz_assert(t);

    // Skip if not a subclass of AssetEditor
    if(!t->IsCastableTo(typeof(PropertyEditor))) continue;

    // Skip the type if it has no allocator
    if(!t->HasAllocator()) continue;

    // Retrieve the importer extension name    
    Name property_name = t->GetMeta(MetaEditorProperty);
    if(property_name.IsEmpty()) {
      Console::WriteLine("%s: error: missing 'EditorProperty' attribute on PropertyEditor'", t->GetQualifiedName().ToCString());
      continue;
    }

    Type* property_type = Type::FindType(property_name);
    if(nullptr==property_type) {
      Console::WriteLine("%s: error: unknown type '%s' for 'EditorProperty' on PropertyEditor'", t->GetQualifiedName().ToCString(),property_name.ToCString());
      continue;
    }

    Name data_name = t->GetMeta(MetaEditorData);
    Type* data_type = nullptr;
    if(!data_name.IsEmpty()) {
      data_type = Type::FindType(data_name);
      if(nullptr==data_type) {
        Console::WriteLine("%s: error: unknown type '%s' for 'EditorData' on PropertyEditor'", t->GetQualifiedName().ToCString(),data_name.ToCString());
        continue;
      }
    }

    RegisteredPropertyEditor pe;
    pe.data_type_ = data_type;
    pe.editor_type_ = t;
    pe.property_type_ = property_type;
    property_editors_.push_back(pe);
  }
}

Sprite* EditorFactory::CreateTypeIcon (Type* type) {
  for(;type;type=type->GetBase()) {
    const Name& icon = type->GetMeta(MetaEditorIcon);
    if(icon.IsEmpty()) continue;

    Sprite* sprite = AssetManager::LoadAsset<Sprite>(Guid::Parse(icon));
    if(sprite) return sprite;
  }

  return nullptr;
}

void EditorFactory::RegisterInspectors (void) {
  // Iterate over all known types and find any that are a subclass of AssetEditor
  const std::vector<Type*>& types = Type::GetTypes();
  for(auto it=types.begin(); it!=types.end(); it++) {
    Type* t = *it;
    noz_assert(t);

    // Skip the base InspectorEditor type
    if(t == typeof(Inspector)) continue;

    // Skip if not a subclass of InspectorEditor
    if(!t->IsCastableTo(typeof(Inspector))) continue;

    // Skip the type if it has no allocator
    if(!t->HasAllocator()) continue;

    // Retrieve the importer extension name    
    Name editor_target = t->GetMeta(MetaEditorTarget);
    if(editor_target.IsEmpty()) {
      Console::WriteLine("%s: error: missing 'EditorTarget' attribute on InspectorEditor'", t->GetQualifiedName().ToCString());
      continue;
    }

    Type* editor_target_type = Type::FindType(editor_target);
    if(nullptr==editor_target_type) {
      Console::WriteLine("%s: error: unknown type '%s' for EditorTarget on InspectorEditor'", t->GetQualifiedName().ToCString(),editor_target.ToCString());
      continue;
    }

    RegisteredInspector ri;
    ri.inspector_type_ = t;
    ri.target_type_ = editor_target_type;
    inspectors_.push_back(ri);
  }
}

Inspector* EditorFactory::CreateInspector (Object* target) {
  if(nullptr == target) return nullptr;
  
  Type* target_type = target->GetType();

  RegisteredInspector* result = nullptr;
  for(noz_uint32 i=0,c=this_->inspectors_.size(); i<c; i++) {
    RegisteredInspector& ri = this_->inspectors_[i];
    if(target_type->IsCastableTo(ri.target_type_)) {
      if(nullptr==result || ri.target_type_->IsCastableTo(result->target_type_)) {
        result = &ri;
      }
    }
  }

  Inspector* inspector = nullptr;

  // Use default editor if no custom editors were found.
  if(nullptr == result) {
    inspector = new Inspector;

  } else {
    inspector = result->inspector_type_->CreateInstance<Inspector>();
    if(nullptr == inspector) {
      Console::WriteLine("error: failed to create inspector editor of type '%s' (using default inspector editor)", result->inspector_type_->GetQualifiedName().ToCString());
      inspector = new Inspector;
    }
  }

  noz_assert(inspector);

  inspector->SetTarget(target);

  return inspector;
}



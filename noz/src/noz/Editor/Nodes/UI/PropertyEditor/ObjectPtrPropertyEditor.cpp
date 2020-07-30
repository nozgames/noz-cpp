///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Actions/SetPropertyAction.h>
#include "ObjectPtrPropertyEditor.h"

using namespace noz;
using namespace noz::Editor;

ObjectPtrPropertyEditor::ObjectPtrPropertyEditor(void) {  
  SetDragDropTarget(true);
}

bool ObjectPtrPropertyEditor::OnApplyStyle (void) {
  return PropertyEditor::OnApplyStyle();      
}

void ObjectPtrPropertyEditor::WriteProperty (Object* t, Property* p) {
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::ObjectPtrProperty)));

  EditorDocument::GetActiveDocument(this)->ExecuteAction(new SetObjectPtrPropertyAction(t,p,value_));
}

void ObjectPtrPropertyEditor::ReadProperty (Object* t, Property* p) {  
  noz_assert(t);
  noz_assert(p);
  noz_assert(p->IsType(typeof(noz::ObjectPtrProperty)));

  SetObject(((ObjectPtrProperty*)p)->Get(t));
}

void ObjectPtrPropertyEditor::SetObject (Object* o) {
  value_ = o;

  if(sprite_node_) sprite_node_->SetSprite(o ? EditorFactory::CreateTypeIcon(o->GetType()) : nullptr);      

  if(text_node_ == nullptr) return;

  if(o==nullptr) {
    text_node_->SetText("None");
    return;
  }

  if(o->GetType()->IsAsset()) {
    AssetFile* file = AssetDatabase::GetFile(((Asset*)o)->GetGuid());
    if(file) {
      text_node_->SetText(file->GetName());
      return;
    }
  } else if(o->IsTypeOf(typeof(Node)) && !((Node*)o)->GetName().IsEmpty()) {
    text_node_->SetText(((Node*)o)->GetName());
  } else {
    text_node_->SetText(String::Format("[%s]",o->GetType()->GetName().ToCString()));  
  }
}

void ObjectPtrPropertyEditor::OnDragDrop (DragDropEventArgs* args) {
  ObjectPtrProperty* p = (ObjectPtrProperty*)GetTargetProperty();
  noz_assert(p);

  // Only support one object..
  if(args->GetObject() && args->GetObject()->IsTypeOf(typeof(ObjectArray)) && ((ObjectArray*)args->GetObject())->GetCount()!=1) return;

  switch(args->GetEventType()) {
    case DragDropEventType::Over:
    case DragDropEventType::Enter:
      if(GetValueFromArgs(args)) args->SetEffects(DragDropEffects::Copy);
      break;

    case DragDropEventType::Drop: {
      Object* o = nullptr;
      if(GetValueFromArgs(args,&o)) {
        SetObject(o);
        PropertyEditor::WriteProperty();
        args->SetEffects(DragDropEffects::Copy);
      }
      break;      
    }
  }
}

void ObjectPtrPropertyEditor::OnSetTarget (void) {
  AssetDatabase::AssetRenamed -= AssetRenamedEvent::Delegate(this, &ObjectPtrPropertyEditor::OnAssetRenamed);

  if(GetValueType()->IsCastableTo(typeof(Asset))) {
    AssetDatabase::AssetRenamed += AssetRenamedEvent::Delegate(this, &ObjectPtrPropertyEditor::OnAssetRenamed);
  }
}

void ObjectPtrPropertyEditor::OnAssetRenamed(AssetFile* asset) {
  noz_assert(GetValueType()->IsCastableTo(typeof(Asset)));

  if(value_ && ((Asset*)(Object*)value_)->GetGuid() != asset->GetGuid()) return;
    
  PropertyEditor::ReadProperty();
}

Type* ObjectPtrPropertyEditor::GetValueType (void) const {
  ObjectPtrProperty* p = (ObjectPtrProperty*)GetTargetProperty();
  noz_assert(p);
  return p->GetObjectType();
}

bool ObjectPtrPropertyEditor::GetValueFromArgs (DragDropEventArgs* args, Object** value) const {
  Type* value_type = GetValueType();
  noz_assert(value_type);

  // If the value is an asset pointer then see if an asset is being dropped
  if(value_type->IsAsset()) {
    AssetFile* file = args->GetObject<AssetFile>();
    if(file && file->GetAssetType()->IsCastableTo(value_type)) {
      if(value) *value = (Object*)AssetManager::LoadAsset(file->GetAssetType(), file->GetGuid());
      return true;
    }
  }

  // Component within a node.
  if(value_type->IsCastableTo(typeof(Component)) && args->GetObject()->IsTypeOf(typeof(Node))) {
    Component* component = ((Node*)args->GetObject())->GetComponent(value_type);
    if(nullptr != component) {
      if(value) *value = component;
      return true;
    }
  }

  // Object being dragged matches the type..
  if(args->GetObject()->GetType()->IsCastableTo(value_type)) {
    if(value) *value = args->GetObject();
    return true;
  }
            
  return false;
}

void ObjectPtrPropertyEditor::Update (void) {
  PropertyEditor::Update();

  if(nullptr==GetTarget()) return;
  if(nullptr==GetTargetProperty()) return;
  
  noz_assert(GetTargetProperty()->IsType(typeof(noz::ObjectPtrProperty)));

  // Look for a change
  Object* o = ((ObjectPtrProperty*)GetTargetProperty())->Get(GetTarget());
  if(value_ != o) {  
    SetObject(o);
  }
}
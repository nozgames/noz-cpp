///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/Button.h>
#include "ObjectPtrVectorPropertyEditor.h"

using namespace noz;
using namespace noz::Editor;


ObjectPtrVectorPropertyEditor::ObjectPtrVectorPropertyEditor(void) {
}

ObjectPtrVectorPropertyEditor::~ObjectPtrVectorPropertyEditor(void) {
}

bool ObjectPtrVectorPropertyEditor::OnApplyStyle (void) {
  if(expander_ && GetTargetProperty()) expander_->SetText(GetTargetProperty()->GetName());
  if(add_button_) add_button_->Click += ClickEventHandler::Delegate(this, &ObjectPtrVectorPropertyEditor::OnAdd);
  if(remove_button_) remove_button_->Click += ClickEventHandler::Delegate(this, &ObjectPtrVectorPropertyEditor::OnRemove);
  RefreshListView();
  return true;
}

void ObjectPtrVectorPropertyEditor::WriteProperty (Object* target, Property* prop) {
}

void ObjectPtrVectorPropertyEditor::ReadProperty (Object* target, Property* prop) {
  RefreshListView();
}

void ObjectPtrVectorPropertyEditor::RefreshListView(void) {
  if(list_view_==nullptr) return;
  noz_uint32 c=((ObjectPtrVectorProperty*)GetTargetProperty())->GetCount(GetTarget());
  for(noz_uint32 i=0;i<c;i++) {
    AddItem(((ObjectPtrVectorProperty*)GetTargetProperty())->Get(GetTarget(),i));
  }
}

Type* ObjectPtrVectorPropertyEditor::GetValueType(void) const {
  return ((ObjectPtrVectorProperty*)GetTargetProperty())->GetObjectType();
}

void ObjectPtrVectorPropertyEditor::AddItem (Object* o) {
  if(nullptr==list_view_) return;

  if(empty_text_) empty_text_->SetVisibility(Visibility::Hidden);
  list_view_->SetVisibility(Visibility::Visible);
  ObjectPtrVectorPropertyEditorItem* item = new ObjectPtrVectorPropertyEditorItem;
  item->editor_ = this;
  list_view_->AddChild(item);
  item->SetObject(o);
}

void ObjectPtrVectorPropertyEditor::OnAdd (UINode*) {
  ((ObjectPtrVectorProperty*)GetTargetProperty())->Add(GetTarget(), nullptr);
  AddItem(nullptr);
}

void ObjectPtrVectorPropertyEditor::OnRemove (UINode*) {
}


ObjectPtrVectorPropertyEditorItem::ObjectPtrVectorPropertyEditorItem(void) {
  SetDragDropTarget(true);
}

void ObjectPtrVectorPropertyEditorItem::SetObject (Object* o) {
  SetSprite(o ? EditorFactory::CreateTypeIcon(o->GetType()) : nullptr);      

  ((ObjectPtrVectorProperty*)editor_->GetTargetProperty())->Remove(editor_->GetTarget(),GetLogicalIndex());
  ((ObjectPtrVectorProperty*)editor_->GetTargetProperty())->Insert(editor_->GetTarget(),GetLogicalIndex(),o);

  if(o==nullptr) {
    SetText("None");
    return;
  }

  if(o->GetType()->IsAsset()) {
    AssetFile* file = AssetDatabase::GetFile(((Asset*)o)->GetGuid());
    if(file) {
      SetText(file->GetName());
      return;
    }
  } else if(o->IsTypeOf(typeof(Node)) && !((Node*)o)->GetName().IsEmpty()) {
    SetText(((Node*)o)->GetName());
  } else {
    SetText(String::Format("[%s]",o->GetType()->GetName().ToCString()));  
  }
}

void ObjectPtrVectorPropertyEditorItem::OnDragDrop(DragDropEventArgs* args) {
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
        args->SetEffects(DragDropEffects::Copy);
      }
      break;      
    }
  }
}

bool ObjectPtrVectorPropertyEditorItem::GetValueFromArgs (DragDropEventArgs* args, Object** value) const {
  Type* value_type = editor_->GetValueType();
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


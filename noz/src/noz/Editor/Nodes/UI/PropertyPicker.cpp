///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "PropertyPicker.h"

#include <noz/Nodes/UI/TreeView.h>
#include <noz/Nodes/UI/TreeViewItem.h>
#include <noz/Nodes/UI/SearchTextBox.h>

using namespace noz;
using namespace noz::Editor;

PropertyPicker::PropertyPicker (void) {
  show_components_ = true;
  show_children_= true;
}

PropertyPicker::~PropertyPicker(void) {
}

bool PropertyPicker::OnApplyStyle(void) {
  if(!Control::OnApplyStyle()) return false;

  if(search_box_) {
    search_box_->TextChanged += ValueChangedEventHandler::Delegate(this,&PropertyPicker::OnSearchTextChanged);
  }

  Refresh();

  //if(list_view_) list_view_->SelectionChanged += SelectionChangedEventHandler::Delegate(this, &PropertyPicker::OnListViewSelectionChanged);

  return true;
}

void PropertyPicker::SetFilterText (const char* text) {
  if(filter_text_.Equals(text,StringComparison::OrdinalIgnoreCase)) return;
  filter_text_ = text;
  Refresh();
}

void PropertyPicker::SetSource (Object* o) {
  if(source_ == o) return;
  source_ = o;
  Refresh();
}

void PropertyPicker::Refresh (void) {
  if(tree_view_==nullptr) return;

  // Clear the tree view.
  tree_view_->RemoveAllChildren();

  if(source_==nullptr) return;

  AddItems (source_,nullptr);
  tree_view_->ExpandAll();
}

void PropertyPicker::AddItems(Object* o, PropertyPickerItem* parent) {
  noz_assert(o);

  PropertyPickerItem* root = new PropertyPickerItem;
  root->SetSprite(EditorFactory::CreateTypeIcon(o->GetType()));
  if(o->IsTypeOf(typeof(Node)) && !((Node*)o)->GetName().IsEmpty()) {
    root->SetText(((Node*)o)->GetName());      
  } else {
    root->SetText(String::Format("[%s]", o->GetType()->GetName().ToCString()));
  }
  if(parent) {
    parent->AddChild(root);
  } else {
    tree_view_->AddChild(root);
  }

  for(Type* t=o->GetType(); t; t=t->GetBase()) {
    for(auto it=t->GetProperties().begin(); it!=t->GetProperties().end(); it++) {
      Property* p = *it;
      if(p->IsTypeOf(typeof(BooleanProperty)) ||
         p->IsTypeOf(typeof(FloatProperty)) ||
         p->IsTypeOf(typeof(Vector2Property)) ) {
        PropertyPickerItem* item = new PropertyPickerItem;
        item->SetSprite(EditorFactory::CreateTypeIcon(o->GetType()));
        item->SetText(p->GetName());
        root->AddChild(item);
      }
    }
  }

  if(o->IsTypeOf(typeof(Node))) {
    Node* n = (Node*)o;
    if(show_components_) {
      if(n->GetTransform()) AddItems(n->GetTransform(), root);

      for(noz_uint32 i=0,c=n->GetComponentCount(); i<c; i++) {
        Component* component = n->GetComponent(i);
        noz_assert(component);
        AddItems(component, root);
      }
    }

    if(show_children_) {
      for(noz_uint32 i=0,c=n->GetLogicalChildCount(); i<c; i++) {
        AddItems(n->GetLogicalChild(i),root);
      }
    }
  }
}

void PropertyPicker::OnSearchTextChanged (UINode* sender) {
  filter_text_ = search_box_->GetText();
  Refresh();
}


PropertyPickerItem::PropertyPickerItem(Object* t, Property* p) : 
  target_(t),
  target_property_(p) {

}


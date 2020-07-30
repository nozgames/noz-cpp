///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/ToggleButton.h>
#include "Hierarchy.h"
#include "HierarchyItem.h"

using namespace noz;
using namespace noz::Editor;


HierarchyItem::HierarchyItem(Hierarchy* hierarchy, Node* target) {
  target_ = target;
  hierarchy_ = hierarchy;

  // Item is a drag drop target
  SetDragDropTarget();
  SetDragDropSource();

  // Update the item text using the target
  UpdateText();

  // Automatically set sprite.
  SetSprite(EditorFactory::CreateTypeIcon(target->GetType()));
}

HierarchyItem::~HierarchyItem(void) {
}

void HierarchyItem::UpdateText(void) {
  if(target_->GetName().IsEmpty()) {
    SetText(String::Format("[%s]", target_->GetType()->GetEditorName().ToCString()));
  } else {
    SetText(target_->GetName());
  }
}

bool HierarchyItem::OnApplyStyle (void) {
  if(!TreeViewItem::OnApplyStyle()) return false;

  if(GetParentItem()==nullptr) {
    if(expand_button_) expand_button_->SetVisibility(Visibility::Collapsed);
  }

  return true;
}

void HierarchyItem::DoDragDrop (const std::vector<ObjectPtr<TreeViewItem>>& items) {
  // If there are more than one items selected...
  Object* drag_object = nullptr;
  if(items.size()>1) {
    ObjectArray* objects = new ObjectArray;
    for(noz_uint32 i=0,c=items.size();i<c; i++) {
      (*objects) += ((HierarchyItem*)(TreeViewItem*)items[i])->target_;
    }    
    drag_object = objects;
  } else if(items.size()==1) {
    drag_object = ((HierarchyItem*)(TreeViewItem*)items[0])->target_;
  }

  if(drag_object == nullptr) return;

  DragDropEffects result = DragDrop::DoDragDrop(this, drag_object, DragDropEffects::Copy);

  if(drag_object->IsType(typeof(ObjectArray))) {
    delete drag_object;
  }  
}


void HierarchyItem::OnDragDrop (DragDropEventArgs* args) {
  noz_assert(args);
  noz_assert(hierarchy_);

  hierarchy_->OnItemDragDrop(this, args);
}

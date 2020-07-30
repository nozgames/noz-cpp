///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/TreeView.h>
#include <noz/Nodes/UI/TreeViewItem.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Editor/Nodes/UI/TypePicker.h>
#include "Hierarchy.h"
#include "HierarchyItem.h"

using namespace noz;
using namespace noz::Editor;


Hierarchy::Hierarchy(void) {
  if(!Application::IsInitializing()) {
    EditorApplication::PropertyChanged += PropertyChangedEventHandler::Delegate(this,&Hierarchy::OnPropertyChanged);
  }
}

Hierarchy::~Hierarchy(void) {
}

bool Hierarchy::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == tree_view_) return false;

  tree_view_->SelectionChanged += SelectionChangedEventHandler::Delegate(this,&Hierarchy::OnTreeViewSelectionChanged);
  tree_view_->ItemTextChanged  += TreeViewTextChangedEventHandler::Delegate(this, &Hierarchy::OnTreeViewTextChanged);

  if(add_node_button_) add_node_button_->Click += ClickEventHandler::Delegate(this,&Hierarchy::OnAddNodeButtonClicked);

  Refresh();    

  return true;
}

void Hierarchy::SetTarget (Node* target) {
  if(target_ == target) return;
  target_ = target;
  if(IsStyled()) Refresh();
}

Node* Hierarchy::GetSelected (void) const {
  if(tree_view_==nullptr) return nullptr;
  HierarchyItem* selected = (HierarchyItem*)tree_view_->GetSelectedItem();
  if(nullptr==selected) return nullptr;
  return selected->target_;
}

void Hierarchy::OnKeyDown (SystemEvent* e) {
  Control::OnKeyDown(e);

  switch(e->GetKeyCode()) {
    case Keys::X:
    case Keys::C:
      if(e->IsControl()) {
        Copy ();
        if(e->GetKeyCode()==Keys::X) DeleteSelectedNodes();
        e->SetHandled();
        return;
      }
      break;

    case Keys::V:
      if(e->IsControl()) {
        Paste ();
        e->SetHandled();
        return;
      }
      break;

    case Keys::Delete: {
      DeleteSelectedNodes();
      break;
    }
  }
}

void Hierarchy::Refresh(void) {
  if(tree_view_ == nullptr) return;
  
  // Clear the content
  tree_view_->RemoveAllChildren();

  // Is Hierarchy empty?
  if(target_ == nullptr) return;

  InsertItem (target_,nullptr)->SetExpanded(true);
}

HierarchyItem* Hierarchy::InsertItem(Node* node, TreeViewItem* parent, noz_int32 insert) {
  HierarchyItem* tvitem = new HierarchyItem(this, node);
  tvitem->SetText(NodeToString(node));
  tvitem->SetSprite(EditorFactory::CreateTypeIcon(node->GetType()));
  tvitem->SetEditable (node->IsRenameable());

  for(noz_uint32 i=0,c=node->GetLogicalChildCount(); i<c; i++) {
    InsertItem (node->GetLogicalChild(i), tvitem, -1);
  }

  if(parent) {
    if(-1 == insert) insert = parent->GetLogicalChildCount();
    parent->InsertChild(insert,tvitem);
  } else {
    if(-1 == insert) insert = tree_view_->GetLogicalChildCount();
    tree_view_->AddChild(tvitem);
  }

  return tvitem;
}

String Hierarchy::NodeToString (Node* node) const {
  if(node->GetName().IsEmpty()) {
    return String::Format("[%s]", node->GetType()->GetEditorName().ToCString());
  }
  return node->GetName();
}

void Hierarchy::OnTreeViewSelectionChanged (UINode* sender) {
  SelectionChanged(this);
}

void Hierarchy::OnAddNodeButtonClicked (UINode* sender) {
  if(GetSelected()==nullptr) return;

  Workspace* workspace = Workspace::GetWorkspace(this);
  if(workspace) {
    Type* t = workspace->GetToolBox()->GetSelected();
    if(t) {
      Node* n = t->CreateInstance<Node>();
      if(n) {
        n->SetName(GetUniqueName(t,GetSelected()));
        if(!AddNodeToSelectedItem(n)) {
          n->Destroy();
        }
      }
    }
  }
}


bool Hierarchy::AddNodeToSelectedItem (Node* n) {
  HierarchyItem* selected_item = (HierarchyItem*)tree_view_->GetSelectedItem();
  if(nullptr==selected_item) return false;

  noz_assert(selected_item->target_);
  selected_item->target_->AddChild(n);

  HierarchyItem* tvitem = InsertItem(n,selected_item);
  if(tvitem) {
    selected_item->SetExpanded(true);
    tvitem->SetSelected(true);
  }

  return true;
}

void Hierarchy::Copy (void) {
  // Multiple items?
  if(tree_view_->GetSelectedItems().size() > 1) {
    ObjectArray oa;
    for(noz_uint32 i=0,c=tree_view_->GetSelectedItems().size(); i<c; i++) {
      TreeViewItem* tvitem = (TreeViewItem*)tree_view_->GetSelectedItems()[i];
      // Dont copy items if a parent is already being copied
      bool skip = false;
      for(TreeViewItem* p=tvitem->GetParentItem(); !skip && p; p=p->GetParentItem()) skip = p->IsSelected();
      if(skip) continue;
      oa += ((HierarchyItem*)tvitem)->target_;
    }
    if(oa.GetCount()==0) return;
    Application::SetClipboard(&oa);
    return;
  } 
  
  // Single selected item?
  if(tree_view_->GetSelectedItem()) {
    Application::SetClipboard(((HierarchyItem*)tree_view_->GetSelectedItem())->target_);
    return;
  }  
}

void Hierarchy::Paste (void) {
  noz_assert(tree_view_);

  // Cannot paste if there is no selection anchor.
  if(nullptr == tree_view_->GetSelectionAnchorItem()) return;

  // Determine if there is a valid clipboard item to paste
  Object* o = Application::GetClipboard(typeof(Node));
  if(nullptr == o) {
    o = Application::GetClipboard(typeof(ObjectArray));
  }

  // Paste the item
  Paste(o, (HierarchyItem*)tree_view_->GetSelectionAnchorItem(), -1);
}

void Hierarchy::Paste (Object* o, HierarchyItem* parent, noz_int32 insert) {
  if(nullptr == o) return;
  if(nullptr == parent) return;

  if(insert == -1) insert = parent->GetChildCount();

  // Node
  if(o->IsTypeOf(typeof(Node))) {
    Node* n = (Node*)o;

    // Insert the hierarchy tiem
    HierarchyItem* item = InsertItem(n, parent, insert);
    
    // Insert the node
    parent->target_->InsertChild(insert,n);

    // Expand the parent item
    parent->SetExpanded(true);

    // Select the new item
    item->SetSelected(true);
    
    return;
  }

  // ObjectArray
  if(o->IsTypeOf(typeof(ObjectArray))) {
    const ObjectArray& oa = *((ObjectArray*)o);
    for(noz_uint32 i=0, c=oa.GetCount(); i<c; i++) {
      if(!oa[i]->IsTypeOf(typeof(Node))) {
        delete oa[i];
        continue;
      }
      Paste(oa[i], parent, insert++);
    }
    delete o;
    return;
  }
}

bool Hierarchy::DeleteSelectedNodes (void) {
  for(Node* n=tree_view_->GetSelectedItem();n;n=tree_view_->GetSelectedItem()) {
    HierarchyItem* item = (HierarchyItem*)n;
    item->target_->Destroy();
    item->target_ = nullptr;

    // Orphan and destroy the node
    n->Destroy();    
  }

  SelectionChanged(this);
  
  return true;
}

DragDropEffects Hierarchy::GetDragDropEffect (Object* o, Node* target) {
  if(o==nullptr) return DragDropEffects::None;

  // DragDropObjects 
  if(o->IsType(typeof(ObjectArray))) {
    ObjectArray* objects = (ObjectArray*)o;
    DragDropEffects result = DragDropEffects::None;
    for(noz_uint32 i=0,c=objects->GetCount();i<c;i++) {
      DragDropEffects effects = GetDragDropEffect((*objects)[i], target);
      switch(effects) {
        case DragDropEffects::None: return DragDropEffects::None;
        case DragDropEffects::Copy: result = DragDropEffects::Copy; break;
        case DragDropEffects::Move: if(result==DragDropEffects::None) result=DragDropEffects::Move; break;
      }
    }
    return result;
  }

  // Type
  if(o->IsTypeOf(typeof(TypePickerItem))) {
    Type* t = (Type*)((TypePickerItem*)o)->GetUserData();
    if(!t->IsCastableTo(typeof(Node))) return DragDropEffects::None;
    return DragDropEffects::Copy;
  }

  // Node
  if(o->IsTypeOf(typeof(Node))) {
    Node* n = (Node*)o;
    if(target->IsDescendantOf(n) || target==n) return DragDropEffects::None;
    return DragDropEffects::Move;
  }

  // AssetFile
  if(o->IsTypeOf(typeof(AssetFile))) {
    AssetFile* file = (AssetFile*)o;
    if(file->CanCreateNode()) {
      return DragDropEffects::Copy;
    }
    return DragDropEffects::None;
  }

  return DragDropEffects::None;
}


void Hierarchy::OnItemDragDrop (HierarchyItem* item, DragDropEventArgs* args) {
  Object* drop_object = args->GetObject();
  if(drop_object==nullptr) return;

  // Convienence variable
  Node* target = item->target_;
  noz_assert(target);

  // Determine the insert direction 
  noz_int32 dir = 0;
  Node* drop_target_node;
  Name state = UI::StateDragDropNone;
  bool isRoot = item == tree_view_->GetFirstChildItem();
  if(!isRoot && item->drag_before_ && item->drag_before_->HitTest(args->GetPosition())==HitTestResult::Rect) {
    state = UI::StateDragDropBefore;
    dir = -1;
    drop_target_node = target->GetLogicalParent();    
  } else if(item->drag_after_ && item->drag_after_->HitTest(args->GetPosition())==HitTestResult::Rect) {
    dir = 1;
    if(item->IsExpanded() && item->HasLogicalChildren()) {
      state = UI::StateDragDropInto;
      drop_target_node = target;
    } else {
      state = UI::StateDragDropAfter;
      drop_target_node = target->GetLogicalParent();
    }
  } else {
    state = UI::StateDragDropInto;
    drop_target_node = target;
  }

  // Get the effect using the drag object and its target location
  DragDropEffects effects = GetDragDropEffect(drop_object, drop_target_node);

  // Promote move effect to copy if control is down.
  if(effects == DragDropEffects::Move && args->IsControl()) effects = DragDropEffects::Copy;
  if(effects == DragDropEffects::None) state = UI::StateDragDropNone;

  // Return the effects
  args->SetEffects(effects);
      
  switch(args->GetEventType()) {
    case DragDropEventType::Enter:
    case DragDropEventType::Over:
      item->SetAnimationState(state);
      break;

    case DragDropEventType::Leave:
      item->SetAnimationState(UI::StateDragDropNone);
      break;

    case DragDropEventType::Drop: {
      // Return back to none state
      item->SetAnimationState(UI::StateDragDropNone);

      // If the drag drop failed just return
      if(effects == DragDropEffects::None) return;

      // Build a list of the items that are being dragged.
      std::vector<HierarchyItem*> items;
      if(drop_object->IsType(typeof(ObjectArray))) {
        ObjectArray* objects = (ObjectArray*)drop_object;
        items.reserve(objects->GetCount());
        for(noz_uint32 i=0,c=objects->GetCount();i<c;i++) {
          if((*objects)[i]->IsTypeOf(typeof(Node))) {
            HierarchyItem* item = FindItem(nullptr,(Node*)(*objects)[i]);
            if(item) items.push_back(item);
          }
        }
      } else if (drop_object->IsTypeOf(typeof(TypePickerItem))) {
        Type* t = (Type*)((TypePickerItem*)drop_object)->GetUserData();
        Node* node = t->CreateInstance<Node>();
        if(node==nullptr) return;
        HierarchyItem* item = new HierarchyItem(this, node);
        items.reserve(1);
        items.push_back(item);
      } else if(drop_object->IsTypeOf(typeof(Node))) {
        items.reserve(1);
        HierarchyItem* item = FindItem(nullptr,(Node*)drop_object);
        if(item) items.push_back(item);
      } else if(drop_object->IsTypeOf(typeof(AssetFile))) {
        AssetFile* file = (AssetFile*)drop_object;
        Node* node = file->CreateNode();
        if(node==nullptr) return;
        HierarchyItem* item = new HierarchyItem(this, node);
        items.reserve(1);
        items.push_back(item);
      }
      
      if(effects == DragDropEffects::Copy) {
        // Copy all nodes being moved
        for(noz_uint32 i=0,c=items.size(); i<c; i++) {
          noz_assert(items[i]);
          noz_assert(items[i]->target_);

          if(!items[i]->IsOrphan()) {
            NOZ_TODO("Create a new item and copy of node");
          }
        }
        
        effects = DragDropEffects::Move;        
      }

      if(effects == DragDropEffects::Move) {
        noz_assert(drop_target_node);

        HierarchyItem* parent_item = FindItem(nullptr,drop_target_node);
        noz_assert(parent_item);

        // Orphan all nodes that are being moved
        for(noz_uint32 i=0,c=items.size(); i<c; i++) {
          noz_assert(items[i]);
          noz_assert(items[i]->target_);
          items[i]->target_->Orphan();
          items[i]->Orphan();
        }

        // Determine the insert index
        noz_uint32 insert;
        if(dir==0) insert=drop_target_node->GetLogicalChildCount();
        else if(dir<0) insert=target->GetLogicalIndex();
        else if(dir>0) insert=target->GetLogicalIndex()+1;

        // Parent all nodes to new parent
        for(noz_uint32 i=0,c=items.size(); i<c; i++, insert++) {
          noz_assert(items[i]);
          noz_assert(items[i]->target_);
          noz_assert(parent_item != items[i]);
          drop_target_node->InsertChild(insert,items[i]->target_);
          parent_item->InsertChild(insert,items[i]);
        }
      }
      break;
    }
  }
}

HierarchyItem* Hierarchy::FindItem (HierarchyItem* root, Node* target) {
  if(tree_view_==nullptr) return nullptr;
  if(root==nullptr) root = (HierarchyItem*)tree_view_->GetFirstChildItem();
  if(target==root->target_) return root;
  for(noz_uint32 i=0,c=root->GetLogicalChildCount();i<c;i++) {
    HierarchyItem* item = FindItem((HierarchyItem*)root->GetLogicalChild(i),target);
    if(item != nullptr) return item;
  }
  return nullptr;
}

void Hierarchy::UnselectAll (void) { 
  if(tree_view_) tree_view_->UnselectAll();
}

void Hierarchy::SetSelected (Node* node) {
  HierarchyItem* item = FindItem(nullptr,node);
  while(nullptr==item && node->GetParent()) {
    node = node->GetParent();
    item = FindItem(nullptr,node);
  }
  if(item==nullptr) return;
  tree_view_->SetSelectedItem(item);
  item->BringIntoView();
}

Name Hierarchy::GetUniqueName (Type* t, Node* parent) {
  // If no parent node just use type name.
  if(nullptr==parent) return t->GetEditorName();

  // If no other nodes in the parent are using the editor name then use it
  if(nullptr==parent->FindChild(t->GetEditorName())) {
    return t->GetEditorName();
  }

  for(noz_uint32 i=1;;i++) {
    Name name = String::Format("%s (%d)", t->GetEditorName().ToCString(), i);
    if(nullptr==parent->FindChild(name)) {
      return name;
    }
  }

  return Name::Empty;  
}

void Hierarchy::OnTreeViewTextChanged(UINode* sender, TreeViewItem* tvitem, const String* text) {
  noz_assert(sender==tree_view_);
  noz_assert(tvitem);
  noz_assert(text);

  HierarchyItem* item = ((HierarchyItem*)tvitem);

  item->target_->SetName(text->ToCString());

  PropertyChangedEventArgs args(item->target_->GetProperty("Name"),item->target_);
  EditorApplication::PropertyChanged(&args);
}

void Hierarchy::OnPropertyChanged(PropertyChangedEventArgs* args) {
  noz_assert(args);
  noz_assert(args->GetTarget());

  if(!args->GetTarget()->IsTypeOf(typeof(Node))) return;

  HierarchyItem* item = FindItem(nullptr, ((Node*)args->GetTarget()));
  if(nullptr == item) return;

  // Name changed..
  if(args->GetProperty() == typeof(Node)->GetProperty("Name")) {
    item->UpdateText();
  }
}

noz_uint32 Hierarchy::GetSelectedItemCount (void) const {
  return tree_view_->GetSelectedItems().size();
}

HierarchyItem* Hierarchy::GetSelectedItem (noz_uint32 i) const {
  return (HierarchyItem*)(TreeViewItem*)tree_view_->GetSelectedItems()[i];
}



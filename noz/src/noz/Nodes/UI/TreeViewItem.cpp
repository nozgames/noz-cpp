///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TextBox.h"
#include "TreeViewItem.h"
#include "TreeView.h"
#include "ToggleButton.h"
#include "Spacer.h"
#include "ScrollView.h"

using namespace noz;


TreeViewItem::TreeViewItem (void) {
  expanded_ = false;
  selected_ = false;
  user_data_ = nullptr;
  selectable_= true;
  editable_ = true;
  hover_ = false;
  depth_ = 0;  
  SetLogicalChildrenOnly();
}

TreeViewItem::~TreeViewItem (void) {
  // Clear any selected state 
  if(tree_view_) {
    if(tree_view_->text_edit_pending_ == this) tree_view_->text_edit_pending_ = nullptr;
    SetSelectedInternal(false, false);
    tree_view_->SelectionChanged(tree_view_);
  }

  AdjustFocusForDelete();
}

void TreeViewItem::SetEditable (bool v) {
  if(editable_ == v) return;
  editable_ = v;
  if(!editable_) EndTextEdit(true); 
}

void TreeViewItem::SetSelectable (bool v) {
  if(v == selectable_) return;
  selectable_ = v;
  if(!selectable_ && selected_) SetSelected(false);
}

void TreeViewItem::SetUserData (Object* user_data) {
  user_data_ = user_data;
}

bool TreeViewItem::OnApplyStyle(void) {
  if(!ContentControl::OnApplyStyle()) return false;
  if(nullptr == items_container_) return false;

  // Optional text box for in-place editing
  if(text_box_) text_box_->SetVisibility(Visibility::Collapsed);

  // Optional expand button
  if(expand_button_) expand_button_->Click += ClickEventHandler::Delegate(this,&TreeViewItem::OnButton);

  // Optional spacer for tree depth
  if(spacer_) spacer_->SetCount(depth_);
  
  // Reparent all items
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    items_container_->AddChild(GetLogicalChild(i));
  }

  UpdateExpanded();

  return true;
}

void TreeViewItem::OnStyleChanged(void) {
  ContentControl::OnStyleChanged();

  // Orphan our logical children from their visual parent
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) GetLogicalChild(i)->Orphan(false);

  items_container_ = nullptr;
}

void TreeViewItem::OnButton(UINode* sender) {
  SetExpanded(expand_button_->IsChecked());
}

void TreeViewItem::SetSelected(bool selected) {
  // Ignore if already selected
  if(selected_ == selected) return;

  // Ignore if the item is not associated with a TreeView yet
  if(nullptr == tree_view_) return;

  // Automatically unselect all selected items if the mode is single.
  if(tree_view_->selection_mode_ == SelectionMode::Single) {
    tree_view_->UnselectAllNoEvent();
  }

  SetSelectedInternal(selected,true);
  
  tree_view_->SelectionChanged(tree_view_);

  // Set this item as the focusd item.
  tree_view_->SetFocusedItem(this);
}

void TreeViewItem::SetSelectedInternal (bool selected, bool set_state) {
  if(selected == selected_) return;
  if(nullptr == tree_view_) return;

  // Stop any pending text edit.
  tree_view_->text_edit_pending_ = nullptr;

  // Mark the item as selected
  selected_ = selected;

  // Selected?
  if(selected) {
    // Add to the selection
    tree_view_->selected_items_.push_back(this);

  } else {
    // Remove from the selection.
    for(auto it=tree_view_->selected_items_.begin(); it!=tree_view_->selected_items_.end(); it++) {
      if(*it == this) {
        tree_view_->selected_items_.erase(it);
        break;
      }
    }
  }

  // Update the animation state.
  if(set_state) SetAnimationState(selected_?UI::StateSelected:UI::StateUnSelected);
}

void TreeViewItem::SetExpanded(bool expanded) {
  // Ensure the item is not already expanded.
  if(expanded == expanded_) return;
  
  // Set as expanded.
  expanded_ = expanded;

  // Update the expanded state.
  UpdateExpanded();
  UpdateAnimationState();

  if(expanded_) OnExpanded(); else OnCollapsed();
}

bool TreeViewItem::IsChildItemOf(TreeViewItem* item) const {
  if(nullptr==item) return true;

  for(TreeViewItem* p=parent_item_; p; p=p->parent_item_) {
    if(p==item) return true;
  }
  return false;
}

void TreeViewItem::UpdateExpanded(void) {
  bool has_items = nullptr != GetFirstChildItem();

  // Hide / Show the expand button depending on the number of items.
  if(expand_button_) {
    expand_button_->SetChecked(expanded_);
    if(has_items) {
      expand_button_->SetVisibility(Visibility::Visible);
    } else {
      expand_button_->SetVisibility(Visibility::Hidden);
    }
  }

  // Hide / Show the items node based on number of items and expanded state
  if(items_container_) {
    if(!expanded_ || !has_items) {
      items_container_->SetVisibility(Visibility::Collapsed);
    } else {
      items_container_->SetVisibility(Visibility::Visible);
    }
  }

  UpdateAnimationState();

  // If collapsed and there are any selected items that are children they need to be deselected.
  if(!expanded_ && tree_view_ ) {
    // Check all selected items
    bool was_child_selected = false;
    for(noz_int32 i=(noz_int32)tree_view_->selected_items_.size()-1; i>=0; i--) {
      // If the selected item is a child of this item..
      if(tree_view_->selected_items_[i]->IsChildItemOf(this)) {
        // Clear its selection
        tree_view_->selected_items_[i]->SetSelectedInternal(false,true);
        was_child_selected = true;
      }
    }

    // If a child was selected then select ourselves
    if(was_child_selected) {
      SetSelectedInternal(true,true);
      tree_view_->SelectionChanged(tree_view_);
    }
  }
}

bool TreeViewItem::HandleSelection (noz_uint32 modifiers) {
  noz_assert(tree_view_);

  tree_view_->SetFocusedItem(this);

  bool control = !!(modifiers & Keys::Control);
  bool shift = !!(modifiers & Keys::Shift);

  // Control click to de-select an item?
  if(tree_view_->selection_mode_ == SelectionMode::Extended && IsSelected() && control) {  
    SetSelectedInternal(false,true);
    tree_view_->selection_anchor_ = this;
    return true;
  } 
  
  // Shift select of one or more items
  if (tree_view_->selection_mode_ == SelectionMode::Extended && shift) {
    // Loop through all expanded items in the entire tree.
    bool inside = false;
    bool changed = false;
    for(TreeViewItem* tvitem=tree_view_->GetFirstChildItem(); tvitem; tvitem=tvitem->GetNextVisibleItem()) {
      bool ss = tree_view_->selection_anchor_ == tvitem;
      bool se = tvitem==this;
      bool s = (ss || se || inside);
        
      if(s && !tvitem->IsSelected() && tvitem->selectable_) {
        tvitem->SetSelectedInternal(true,true);
        changed = true;
      } else if (!s && tvitem->IsSelected()) {
        tvitem->SetSelectedInternal(false,true);
        changed = true;
      }

      if((ss && se) || ((ss || se) && inside)) {
        inside = false;
      } else {
        inside = s;
      }
    } 

    return changed;
  }

  // Toggle selection
  if(tree_view_->selection_mode_==SelectionMode::Multiple && !IsSelected()) {
    SetSelectedInternal(!selected_,true);
    tree_view_->selection_anchor_ = this;
    return true;
  }

  // Select if not already selected
  if(!IsSelected() || tree_view_->GetSelectedItems().size() > 1) {
    // Unselect if control is not pressed
    if(tree_view_->selection_mode_!=SelectionMode::Extended || !control) tree_view_->UnselectAllNoEvent();

    SetSelectedInternal(true,true);
    tree_view_->selection_anchor_ = this;
    return true;
  }

  return false;
}

void TreeViewItem::OnMouseDown (SystemEvent* e) {
  noz_assert(e);

  // Ensure the item is linked to a tree ivew
  if(nullptr==tree_view_) return;

  // Ensure the item is selectable
  if(!selectable_) return;
  
  // Let base get a crack at event first
  Control::OnMouseDown(e);

  // Only care about left mouse button
  if(e->GetButton() != MouseButton::Left) return;

  // Alwasy set focus to the tree view on a click
  bool had_focus = tree_view_->HasFocus();
  tree_view_->SetFocus();

  // Save the current selected items list if this item is a drag source
  if(IsDragDropSource()) TreeView::saved_selected_items_ = tree_view_->selected_items_;

  // Handle the click.
  bool selection_changed = HandleSelection(e->GetModifiers());

  // If there was no change in selection then there is no need for the 
  // saved selected items as there is no selection to restore.  Doing this
  // also signals the MouseUp method that it does not need to broadcast
  // a selection changed event since the DoDragDrop method will also clear this 
  if(!selection_changed) TreeView::saved_selected_items_.clear();

  // Mark event as handled.
  e->SetHandled();

  // Send SelectionChanged event if not a drag drop srouce and the selection changed.
  if(!IsDragDropSource() && (selection_changed || TreeView::saved_selected_items_.empty())) {
    tree_view_->SelectionChanged(tree_view_);
  }

  // Double click should toggle expand state
  if(e->GetClickCount()==2) {
    tree_view_->text_edit_pending_ = nullptr;
    SetExpanded(!IsExpanded());
    return;
  }

  // If the selection did not change and the text node was clicked on start in place editing.
  if(e->GetModifiers()==0 && had_focus && !selection_changed && text_node_ && text_node_->HitTest(e->GetPosition())==HitTestResult::Rect) {
    BeginTextEdit(true);
  }
}

void TreeViewItem::OnMouseUp (SystemEvent* e) {
  Control::OnMouseUp(e);

  if(IsDragDropSource()&&!TreeView::saved_selected_items_.empty()) {
    tree_view_->SelectionChanged(tree_view_);
  }
}

void TreeViewItem::OnMouseOver (SystemEvent* e) {
  Control::OnMouseOver(e);

  if(hover_ != (nullptr == items_container_ || !items_container_->IsMouseOver())) UpdateAnimationState();
}

void TreeViewItem::DoDragDrop (void) {  
  // Perform the drag drop
  DoDragDrop(tree_view_->selected_items_);

  // Restore the selection prior to the drag
  tree_view_->UnselectAllNoEvent();
  for(noz_uint32 i=0,c=TreeView::saved_selected_items_.size(); i<c; i++) {
    TreeViewItem* tvitem = TreeView::saved_selected_items_[i];
    if(tvitem) tvitem->SetSelectedInternal(true,true);            
  }

  // Clear the saved list to 
  TreeView::saved_selected_items_.clear();
}

void TreeViewItem::SetTreeView (TreeView* tv, TreeViewItem* parent_item) {
  // Ensure we dont already have the most up to date information.
  if(parent_item_ == parent_item && tv == tree_view_ && (parent_item && depth_ == parent_item->depth_ + 1)) return;

  if(tv == nullptr) {
    if(selected_) SetSelectedInternal(false,true);
    depth_ = 0;
    parent_item = nullptr;
    tree_view_ = nullptr;
    return;
  }

  // Update the depth and tree view
  depth_ = parent_item ? parent_item->depth_ + 1 : 0;
  parent_item_ = parent_item;
  tree_view_ = tv;

  // If there is a spacer in the item then set its count to the depth.  The spacer is used   
  // to allow full row select in treeviews if desired.
  if(spacer_) spacer_->SetCount(depth_);

  // Update all of the child items as well
  for(TreeViewItem* tvchild=GetFirstChildItem(); tvchild; tvchild=tvchild->GetNextSiblingItem()) tvchild->SetTreeView(tv,this);

  UpdateExpanded();
  UpdateAnimationState();
}

void TreeViewItem::AdjustFocusForDelete(void) {
  if(tree_view_ == nullptr) return;
  if(tree_view_->focus_item_!=this) return;

  tree_view_->focus_item_ = GetNextVisibleItem();
  if(nullptr == tree_view_->focus_item_) {
    tree_view_->focus_item_ = GetPrevVisibleItem();
  }
}

TreeViewItem* TreeViewItem::GetFirstChildItem (void) const {
  for(noz_uint32 i=0,c=GetLogicalChildCount(); i<c; i++) {
    Node* child = GetLogicalChild(i);
    if(child->IsTypeOf(typeof(TreeViewItem))) return (TreeViewItem*)child;
  }
  return nullptr;
}

TreeViewItem* TreeViewItem::GetLastChildItem (void) const {
  for(noz_uint32 i=GetLogicalChildCount(); i>0; i--) {
    Node* child = GetLogicalChild(i-1);
    if(child->IsTypeOf(typeof(TreeViewItem))) return (TreeViewItem*)child;
  }
  return nullptr;
}

TreeViewItem* TreeViewItem::GetNextSiblingItem (void) const {
  for(Node* n=GetNextLogicalSibling(); n; n=n->GetNextLogicalSibling()) {
    if(n->IsTypeOf(typeof(TreeViewItem))) return (TreeViewItem*)n;
  }
  return nullptr;
}

TreeViewItem* TreeViewItem::GetPrevSiblingItem (void) const {
  for(Node* n=GetPrevLogicalSibling(); n; n=n->GetPrevLogicalSibling()) {
    if(n->IsTypeOf(typeof(TreeViewItem))) return (TreeViewItem*)n;
  }
  return nullptr;
}

TreeViewItem* TreeViewItem::GetNextVisibleItem(void) const {
  if(IsExpanded()) {
    TreeViewItem* tvitem = GetFirstChildItem();
    if(tvitem) return tvitem;
  }

  TreeViewItem* tvitem = GetNextSiblingItem();
  if(nullptr != tvitem) {
    return tvitem;
  }

  for(tvitem=GetParentItem(); tvitem; tvitem = tvitem->GetParentItem()) {
    TreeViewItem* tvitem_next = tvitem->GetNextSiblingItem();
    if(tvitem_next) return tvitem_next;
  }

  return nullptr;
}

TreeViewItem* TreeViewItem::GetPrevVisibleItem(void) const {
  TreeViewItem* tvitem = GetPrevSiblingItem();
  if(nullptr == tvitem) return GetParentItem();
  if(nullptr == tvitem) return nullptr;

  while(tvitem->IsExpanded()) {
    TreeViewItem* tvitem_child = tvitem->GetLastChildItem();
    if(nullptr==tvitem_child) break;
    tvitem = tvitem_child;
  }

  return tvitem;
}

bool TreeViewItem::IsFocused (void) const {
  return tree_view_ && tree_view_->HasFocus() && tree_view_->focus_item_ == this;
}

void TreeViewItem::UpdateAnimationState (void) {
  // Expanded state
  SetAnimationState(expanded_ ? UI::StateExpanded : UI::StateCollapsed);

  // Disabled?
  if(!IsInteractive()) {
    SetAnimationState(UI::StateDisabled);
    SetAnimationState(UI::StateUnSelected);
    SetAnimationState(UI::StateUnFocused);
    return;
  }

  // Mouse over?
  hover_ = IsMouseOver() && !(items_container_ && items_container_->IsMouseOver());
  SetAnimationState(hover_ ? UI::StateMouseOver : UI::StateNormal);

  // Selected state
  if(selected_) {
    if(tree_view_&&tree_view_->HasFocus()) {      
      SetAnimationState(UI::StateSelected);
    } else {
      SetAnimationState(UI::StateSelectedUnFocused);
    }
  } else {
    SetAnimationState(UI::StateUnSelected);
  }

  // Focused state
  SetAnimationState(IsFocused()  ? UI::StateFocused : UI::StateUnFocused);
}

void TreeViewItem::BringIntoView (void) {
  // First expand the item.
  for(TreeViewItem* tvitem = this;tvitem;tvitem=tvitem->parent_item_) tvitem->SetExpanded(true);
  
  tree_view_->scroll_view_->BringIntoView(this);
}

void TreeViewItem::OnChildAdded(Node* child) {
  ContentControl::OnChildAdded(child);

  if(!child->IsPrivate() && child->IsTypeOf(typeof(TreeViewItem))) {
    ((TreeViewItem*)child)->SetTreeView(tree_view_,this);
  }

  if(items_container_) items_container_->InsertChild(child->GetLogicalIndex(), child);

  UpdateExpanded();
  UpdateAnimationState();
}

void TreeViewItem::OnChildRemoved(Node* child) {
  ContentControl::OnChildRemoved(child);

  if(!child->IsPrivate() && child->IsTypeOf(typeof(TreeViewItem))) {
    ((TreeViewItem*)child)->SetTreeView(nullptr, nullptr);
  }

  UpdateExpanded();
  UpdateAnimationState();
}

void TreeViewItem::ExpandAll (void) {
  SetExpanded(true);

  // Expand all child items
  for(TreeViewItem* tvitem=GetFirstChildItem(); tvitem; tvitem=tvitem->GetNextSiblingItem()) {
    tvitem->ExpandAll();
  }
}

SelectionMode TreeViewItem::GetSelectionMode (void) const {
  if(tree_view_) return tree_view_->selection_mode_;
  return SelectionMode::None;
}

bool TreeViewItem::BeginTextEdit (bool delay) {
  if(!editable_) return false;

  if(nullptr==text_node_ || nullptr == text_box_) {
    Measure(Vector2::Empty);
    if(nullptr==text_node_ || nullptr == text_box_) {
      return false;
    }
  }

  if(!delay) {
    tree_view_->text_edit_pending_ = nullptr;

    noz_assert(text_node_);
    noz_assert(text_box_);

    text_node_->SetVisibility(Visibility::Hidden);
    text_box_->LostFocus += LostFocusEventHandler::Delegate(this,&TreeViewItem::OnTextBoxLostFocus);
    text_box_->TextCommited += ValueChangedEventHandler::Delegate(this,&TreeViewItem::OnTextBoxTextCommited);
    text_box_->SetVisibility(Visibility::Visible);
    text_box_->SetText(text_node_->GetText());
    text_box_->SetFocus();  
  } else {
    tree_view_->text_edit_pending_ = this;
  }

  return true;
}

void TreeViewItem::EndTextEdit(bool cancel) {
  if(nullptr==text_node_ || nullptr == text_box_) return;
  if(text_box_->GetVisibility() == Visibility::Collapsed) return;

  text_box_->SetVisibility(Visibility::Collapsed);
  text_box_->LostFocus -= LostFocusEventHandler::Delegate(this,&TreeViewItem::OnTextBoxLostFocus);
  text_box_->TextCommited -= ValueChangedEventHandler::Delegate(this,&TreeViewItem::OnTextBoxTextCommited);
  text_node_->SetVisibility(Visibility::Visible);
  if(!cancel && !text_box_->GetText().Equals(text_node_->GetText())) {
    tree_view_->ItemTextChanged(tree_view_,this,&text_box_->GetText());
  }
}

void TreeViewItem::OnTextBoxLostFocus(UINode* sender) {
  EndTextEdit();
}

void TreeViewItem::OnTextBoxTextCommited(UINode* sender) {
  EndTextEdit();
}

void TreeViewItem::OnPreviewKeyDown (SystemEvent* e) { 
  // If the text is being edited and ESC is pressed cancel the edit
  if(text_box_ && text_box_->GetVisibility() == Visibility::Visible && e->GetKeyCode() == Keys::Escape) {
    EndTextEdit(true);
    e->SetHandled();
    return;
  }
  ContentControl::OnPreviewKeyDown(e);
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/UI/DragDrop.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include <noz/Nodes/UI/TextBox.h>
#include "ListViewItem.h"
#include "ListView.h"
#include "ScrollView.h"

using namespace noz;


ListViewItem::ListViewItem (void) {
  list_view_ = nullptr;
  selected_ = false;
  user_data_= nullptr;
  text_edit_pending_ = false;
}

void ListViewItem::BringIntoView (void) { 
  if(list_view_ && list_view_->scroll_view_) list_view_->scroll_view_->BringIntoView(this);
}

void ListViewItem::SetSelected(bool selected) {
  if(selected_ == selected) return;
  if(nullptr == list_view_) return;

  SetSelectedInternal(selected,true);

  // Change the focus item
  list_view_->SetFocusedItem(this, true);

  // Fire selection changed event.
  list_view_->SelectionChanged(list_view_);
}

void ListViewItem::SetSelectedInternal (bool selected, bool set_state) {
  if(selected == selected_) return;
  if(nullptr == list_view_) return;

  text_edit_pending_ = false;

  // Mark the item as selected
  selected_ = selected;

  // Selected?
  if(selected) {
    // Add to the selection
    list_view_->selected_items_.push_back(this);

  } else {
    // Remove from the selection.
    for(auto it=list_view_->selected_items_.begin(); it!=list_view_->selected_items_.end(); it++) {
      if(*it == this) {
        list_view_->selected_items_.erase(it);
        break;
      }
    }
  }

  // Update the animation state.
  if(set_state) UpdateAnimationState();
}

ListViewItem* ListViewItem::GetNextSiblingItem (void) const {
  for(Node* n=GetNextLogicalSibling(); n; n=n->GetNextLogicalSibling()) {
    if(n->IsTypeOf(typeof(ListViewItem))) return (ListViewItem*)n;
  }
  return nullptr;
}

ListViewItem* ListViewItem::GetPrevSiblingItem (void) const {
  for(Node* n=GetPrevLogicalSibling(); n; n=n->GetPrevLogicalSibling()) {
    if(n->IsTypeOf(typeof(ListViewItem))) return (ListViewItem*)n;
  }
  return nullptr;
}

void ListViewItem::OnMouseDown (SystemEvent* e) {
  noz_assert(e);

  Control::OnMouseDown(e);

  // If not associated with a list view then ignore all input
  if(nullptr == list_view_) return;

  // If not left button then ignore
  if(e->GetButton() != MouseButton::Left) return;

  // Ignore double clicks
  if(e->GetClickCount() != 1) {
    text_edit_pending_ = false;
    return;
  }

  // Alwasy set focus to the list view when clicked on.
  bool had_focus = list_view_->HasFocus();
  list_view_->SetFocus();

  // Update the current selection using the modifiers
  bool selection_changed = HandleSelection (e->GetModifiers());    

  e->SetHandled();

  if(selection_changed) {
    list_view_->SelectionChanged(list_view_);
  } else if(e->GetModifiers()==0 && had_focus && text_node_ && text_node_->HitTest(e->GetPosition())==HitTestResult::Rect) {
    BeginTextEdit(true);
  }
}

void ListViewItem::Update (void) {
  ContentControl::Update();

  // Handle text editing.
  if(!text_edit_pending_) return;

  // If the mouse is being dragged cancel the edit
  if(Input::GetMouseButtonDrag(MouseButton::Left)) {
    text_edit_pending_ = false;
    return;
  }

  if(Time::GetTime() - Input::GetMouseButtonTime(MouseButton::Left) < 0.5f) return;

  BeginTextEdit();  
}

bool ListViewItem::HandleSelection (noz_uint32 modifiers) {
  noz_assert(list_view_);

  bool control = !!(modifiers & Keys::Control);
  bool shift = !!(modifiers & Keys::Shift);  

  // Control click to de-select an item?
  if(list_view_->selection_mode_ == SelectionMode::Extended && IsSelected() && control) {  
    SetSelectedInternal(false,true);
    list_view_->SetFocusedItem(this,true);
    return true;
  } 
  
  // Shift select of one or more items
  if (list_view_->selection_mode_ == SelectionMode::Extended && shift) {
    // Loop through all expanded items in the entire tree.
    bool inside = false;
    bool changed = false;
    for(ListViewItem* lvitem=list_view_->GetFirstChildItem(); lvitem; lvitem=lvitem->GetNextSiblingItem()) {
      bool ss = list_view_->selection_anchor_ == lvitem;
      bool se = lvitem==this;
      bool s = (ss || se || inside);
        
      if(s && !lvitem->IsSelected()) {
        lvitem->SetSelectedInternal(true,true);
        changed = true;
      } else if (!s && lvitem->IsSelected()) {
        lvitem->SetSelectedInternal(false,true);
        changed = true;
      }

      if((ss && se) || ((ss || se) && inside)) {
        inside = false;
      } else {
        inside = s;
      }
    } 

    list_view_->SetFocusedItem(this,false);

    return changed;
  }

  // Toggle selection
  if(list_view_->selection_mode_==SelectionMode::Multiple && !IsSelected()) {
    SetSelectedInternal(!selected_,true);
    list_view_->SetFocusedItem(this,true);
    return true;
  }

  // Select if not already selected
  if(!IsSelected() || list_view_->GetSelectedItems().size() > 1) {
    // Unselect if control is not pressed
    if(list_view_->selection_mode_!=SelectionMode::Extended || !control) list_view_->UnselectAllNoEvent();
    SetSelectedInternal(true,true);
    list_view_->SetFocusedItem(this,true);
    return true;
  }

  return false;
}

bool ListViewItem::BeginTextEdit (bool delay) {
  if(nullptr==text_node_ || nullptr == text_box_) {
    Measure(Vector2::Empty);
    if(nullptr==text_node_ || nullptr == text_box_) {
      return false;
    }
  }

  if(delay) {
    text_edit_pending_ = true;
  } else {
    text_edit_pending_ = false;

    noz_assert(text_node_);
    noz_assert(text_box_);

    text_node_->SetVisibility(Visibility::Hidden);
    text_box_->LostFocus += LostFocusEventHandler::Delegate(this,&ListViewItem::OnTextBoxLostFocus);
    text_box_->TextCommited += ValueChangedEventHandler::Delegate(this,&ListViewItem::OnTextBoxTextCommited);
    text_box_->SetVisibility(Visibility::Visible);
    text_box_->SetText(text_node_->GetText());
    text_box_->SetFocus();
  }

  return true;
}

void ListViewItem::EndTextEdit(bool cancel) {
  if(nullptr==text_node_ || nullptr == text_box_) return;

  text_box_->SetVisibility(Visibility::Collapsed);
  text_box_->LostFocus -= LostFocusEventHandler::Delegate(this,&ListViewItem::OnTextBoxLostFocus);
  text_box_->TextCommited -= ValueChangedEventHandler::Delegate(this,&ListViewItem::OnTextBoxTextCommited);
  text_node_->SetVisibility(Visibility::Visible);
  if(!cancel && !text_box_->GetText().Equals(text_node_->GetText())) {
    list_view_->ItemTextChanged(list_view_,this,&text_box_->GetText());
  }
}

void ListViewItem::OnTextBoxLostFocus(UINode* sender) {
  EndTextEdit();
}

void ListViewItem::OnTextBoxTextCommited(UINode* sender) {
  EndTextEdit();
}

void ListViewItem::OnPreviewKeyDown (SystemEvent* e) { 
  // If the text is being edited and ESC is pressed cancel the edit
  if(text_box_->GetVisibility() == Visibility::Visible && e->GetKeyCode() == Keys::Escape) {
    EndTextEdit(true);
    e->SetHandled();
    return;
  }
  ContentControl::OnPreviewKeyDown(e);
}

void ListViewItem::OnMouseUp (SystemEvent* e) {
  noz_assert(e);
  noz_assert(e->GetEventType() == SystemEventType::MouseUp);

  Control::OnMouseUp(e);

  if(HasCapture()) {
    ReleaseCapture();
    e->SetHandled();
  }
}

void ListViewItem::SetListView (ListView* lv) {
  if(lv == list_view_) return;

  // If already in a list view and selected then deselect the item quietly
  // to remove it from the parent listview selected list.
  if(list_view_ && selected_) SetSelectedInternal(false, false);

  // Save list view pointer.
  list_view_ = lv;
}

void ListViewItem::UpdateAnimationState (void) {
  // Selected state
  if(selected_) {
    if(list_view_&&(list_view_->HasFocus()||(text_box_&&text_box_->HasFocus()))) {      
      SetAnimationState(UI::StateSelected);
    } else {
      SetAnimationState(UI::StateSelectedUnFocused);
    }
  } else{
    SetAnimationState(UI::StateUnSelected);
  }

  // Focused state
  if(list_view_ && list_view_->HasFocus() && list_view_->focus_item_ == this) {
    SetAnimationState(UI::StateFocused);
  } else {
    SetAnimationState(UI::StateUnFocused);
  }

  Control::UpdateAnimationState();
}

SelectionMode ListViewItem::GetSelectionMode (void) const {
  if(list_view_) return list_view_->selection_mode_;
  return SelectionMode::None;
}

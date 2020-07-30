///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "DragDrop.h"

using namespace noz;

ObjectPtr<UINode> DragDrop::drag_node_;
noz_uint32 DragDrop::drag_modifiers_ = 0;
DragDropEffects DragDrop::drag_effects_ = DragDropEffects::None;
bool DragDrop::drag_drop_active_ = false;


UINode* DragDrop::HitTest (Node* node, const Vector2& pos) {
  HitTestResult hit_result = node->HitTest(pos);
  if(hit_result == HitTestResult::None) return nullptr;

  for(noz_uint32 i=node->GetChildCount();i>0;i--) {
    Node* result = HitTest(node->GetChild(i-1), pos);
    if(nullptr != result) return (UINode*)result;
  }

  for(noz_uint32 i=node->GetPrivateChildCount();i>0;i--) {
    Node* result = HitTest(node->GetPrivateChild(i-1), pos);
    if(nullptr != result) return (UINode*)result;
  }

  if(hit_result == HitTestResult::Rect) {
    if(node->IsTypeOf(typeof(UINode)) && ((UINode*)node)->IsDragDropTarget()) return (UINode*)node;
  }

  return nullptr;
}

void DragDrop::HandleEvent (SystemEvent* e) {
  // Need a window...
  Window* window = e->GetWindow();
  if(nullptr == window) return;

  switch(e->GetEventType()) {
    case SystemEventType::DragEnter:
      drag_node_ = nullptr;

    case SystemEventType::DragOver: {
      //noz_assert(e->GetEventType()==SystemEventType::DragOver || drag_node_==nullptr);

      // Determine the node that the mouse if over..
      UINode* hit = HitTest(window->GetRootNode(),e->GetPosition());

      // If the hit node has changed.
      if(hit != drag_node_) {
        if(drag_node_) {
          DragDropEventArgs args(DragDropEventType::Leave, e->GetObject(),e->GetPosition(), e->GetModifiers());
          drag_node_->OnDragDrop(&args);
        }
        drag_effects_ = DragDropEffects::None;
        drag_node_ = hit;
        if(hit) {
          DragDropEventArgs args(DragDropEventType::Enter, e->GetObject(),e->GetPosition(), e->GetModifiers());
          drag_node_->OnDragDrop(&args);
          drag_effects_ = args.GetEffects();
        }
      } else if(hit) {
        DragDropEventArgs args(DragDropEventType::Over, e->GetObject(),e->GetPosition(), e->GetModifiers());
        drag_node_->OnDragDrop(&args);
        drag_effects_ = args.GetEffects();
      }

      break;
    }

    case SystemEventType::DragLeave:
      if(drag_node_) {
        DragDropEventArgs args(DragDropEventType::Leave, e->GetObject(),e->GetPosition(), e->GetModifiers());
        drag_node_->OnDragDrop(&args);
        drag_node_ = nullptr;
      }
      drag_effects_ = DragDropEffects::None;
      break;

    case SystemEventType::Drop:
      if(drag_node_) {
        DragDropEventArgs args(DragDropEventType::Drop, e->GetObject(),e->GetPosition(), e->GetModifiers());
        drag_node_->OnDragDrop(&args);
        drag_effects_ = args.GetEffects();
        drag_node_ = nullptr;
      }
      break;

    default: break;
  }      
}

DragDropEffects DragDrop::DoDragDrop (UINode* source, Object* data, DragDropEffects allowedEffects) {
  // The source node must be part of a window system to allow a drag on it
  Window* window = source->GetWindow();
  if(nullptr == window) return DragDropEffects::None;

  // Set a flag to indicat a drag drop is in progress
  drag_drop_active_ = true;

  DragDropEffects result = DoDragDropImplementation (window, data, allowedEffects);

  // Clear the flag indicating a drag drop is in progress.
  drag_drop_active_ = false;

  return result;
}

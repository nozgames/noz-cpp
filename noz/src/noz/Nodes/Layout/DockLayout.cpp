///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "DockLayout.h"

using namespace noz;

DockLayout::DockLayout(void) {
  dock_ = Dock::Top;
  last_child_fill_ = true;
  spacing_ = 0.0f;
}

void DockLayout::SetDock(Dock dock) {
  if(dock_ == dock) return;
  dock_ = dock;
  InvalidateTransform();
}

void DockLayout::SetSpacing (noz_float spacing) {
  if(spacing_ == spacing) return;
  spacing_ = spacing;
  InvalidateTransform();
}

void DockLayout::SetLastChildFill (bool v) {
  if(last_child_fill_ == v) return;
  last_child_fill_ = v;
  InvalidateTransform();
}

Vector2 DockLayout::MeasureChildren (const Vector2& a) {
  Vector2 size;
  Vector2 ra = a;

  // The U axis is the axis on which content flows normally before wrapping
  noz_int32 u_axis;

  // The V axis is the axis on which the panel grows when a wrap occurrs
  noz_int32 v_axis;

  switch(dock_) {
    case Dock::Bottom:
    case Dock::Top:
      u_axis = 1;
      v_axis = 0;
      break;

    case Dock::Right:
    case Dock::Left:
      u_axis = 0;
      v_axis = 1;
      break;
  }

  noz_uint32 count = 0;
  for(noz_uint32 i=0,c=GetChildCount();i<c;i++) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;

    Vector2 csize=child->Measure(ra);
    ra[u_axis] -= csize[u_axis];
    size[u_axis] += csize[u_axis];
    size[v_axis] = Math::Max(size[v_axis], csize[v_axis]);
    count++;
  }

  if(count>1) size[u_axis] += (spacing_ * (count-1));

  return size;
}
    
void DockLayout::ArrangeChildren(const Rect& r) {
  Rect rect = r;

  // The U axis is the axis on which content flows normally before wrapping
  noz_int32 u_axis;

  // The V axis is the axis on which the panel grows when a wrap occurrs
  noz_int32 v_axis;

  noz_float u_min;
  noz_float u_add_before;
  noz_float u_add_after;
  noz_float u_spacing;

  switch(dock_) {
    case Dock::Bottom:
      u_axis = 1;
      v_axis = 0;
      u_min = rect.y + rect.h;
      u_add_before = -1.0f;
      u_add_after = 0.0f;
      u_spacing = -spacing_;
      break;

    case Dock::Top:
      u_axis = 1;
      v_axis = 0;
      u_min = rect.y;
      u_add_before = 0.0f;
      u_add_after = 1.0f;
      u_spacing = spacing_;
      break;

    case Dock::Right:
      u_axis = 0;
      v_axis = 1;
      u_min = rect.x + rect.w;
      u_add_before = -1.0f;
      u_add_after = 0.0f;
      u_spacing = -spacing_;
      break;

    case Dock::Left:
      u_axis = 0;
      v_axis = 1;
      u_min = rect.x;
      u_add_before = 0.0f;
      u_add_after = 1.0f;
      u_spacing = spacing_;
      break;
  }

  // Start with no height on the rect
  rect[u_axis] = u_min;

  for(noz_uint32 i=0,c=GetChildCount();i<c;i++) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;

    if(i==c-1 && last_child_fill_) {
      rect[u_axis+2] = r[u_axis+2] - Math::Abs(rect[u_axis]-u_min);
    } else {
      rect[u_axis+2] = child->GetMeasuredSize()[u_axis];
    }

    rect[u_axis] += (u_add_before * rect[u_axis+2]);
    child->Arrange(rect);
    rect[u_axis] += (u_add_after * rect[u_axis+2]);
    rect[u_axis] += u_spacing;
  }
}


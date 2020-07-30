///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "StackLayout.h"

using namespace noz;

StackLayout::StackLayout(void) {
  orientation_ = Orientation::Vertical;
  spacing_ = 0.0f;
}

void StackLayout::SetSpacing (noz_float spacing) {
  if(spacing_==spacing) return;
  spacing_ = spacing;
  InvalidateTransform();
}

void StackLayout::SetOrientation(Orientation orientation) {
  if(orientation_ == orientation) return;
  orientation_ = orientation;
  InvalidateTransform();
}

Vector2 StackLayout::MeasureChildren (const Vector2& available_size) {
  Vector2 size;

  // The U axis is the axis which content grows
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis which will have a fixed size
  noz_int32 v_axis = !u_axis;

  noz_uint32 count = 0;
  for(noz_uint32 i=0,c=GetChildCount();i<c;i++) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;

    Vector2 csize = child->Measure(available_size);
    size[v_axis] = Math::Max(size[v_axis],csize[v_axis]);
    size[u_axis] += csize[u_axis];
    count++;
  }

  // Add in the spacing to the measure
  if(count > 1) size[u_axis] += (spacing_ * (count-1));

  return size;
}
    
void StackLayout::ArrangeChildren(const Rect& r) {
  // The U axis is the axis which content grows
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis which will have a fixed size
  noz_int32 v_axis = !u_axis;
  
  // Arrange all children that are not collapsed
  Rect rect = r;
  rect[u_axis+2] = 0;
  
  for(noz_uint32 i=0,c=GetChildCount();i<c;i++) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;
    rect[u_axis+2] = child->GetMeasuredSize()[u_axis];
    child->Arrange(rect);
    rect[u_axis] += (rect[u_axis+2] + spacing_);
  }  
}


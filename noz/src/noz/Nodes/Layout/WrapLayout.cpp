///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "WrapLayout.h"

using namespace noz;

WrapLayout::WrapLayout(void) {
  orientation_ = Orientation::Horizontal;
}

void WrapLayout::SetOrientation(Orientation orientation) {
  if(orientation_ == orientation) return;
  orientation_ = orientation;
  InvalidateTransform();
}

Vector2 WrapLayout::MeasureChildren (const Vector2& available_size) {
  Vector2 size;

  // The U axis is the axis on which content flows before wrapping
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis on which the panel grows when a wrap occurrs
  noz_int32 v_axis = !u_axis;

  // Current size on both U and V axis.
  noz_float u_size = 0.0f;
  noz_float v_size = 0.0f;

  // Consider all visual children
  for(noz_uint32 i=0,c=GetChildCount();i<c;i++) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;

    // Measure the node using all of the available size.
    Vector2 csize = child->Measure(available_size);

    // Did the node cause us to wrap?  
    noz_float msize = csize[u_axis];
    if(u_size > 0.0f && u_size + msize > available_size[u_axis]) {
      size[u_axis] = Math::Max(u_size,size[u_axis]);
      size[v_axis] += v_size;
      u_size = 0.0f;
      v_size = 0.0f;
    }

    u_size += msize;
    v_size = Math::Max(v_size,csize[v_axis]);
  }

  // Ensure remainder is added to size.
  size[u_axis] = Math::Max(u_size,size[u_axis]);
  size[v_axis] += v_size;

  return size;
}
    
void WrapLayout::ArrangeChildren (const Rect& nrect) {
  // The U axis is the axis on which content flows normally before wrapping
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis on which the panel grows when a wrap occurrs
  noz_int32 v_axis = !u_axis;

  // Maximum size of u axis before content will wrap
  noz_float u_max = nrect.GetSize()[u_axis];

  // Offset on V axis for item
  noz_float v_offset = 0.0f;

  for(noz_uint32 i=0,c=GetChildCount();i<c;) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;

    // Index of first item in the line
    noz_uint32 u_first = i;

    // Index of the last item in the line (exclusive)
    noz_uint32 u_last = i+1;

    const Vector2& msize = child->GetMeasuredSize();

    // Initial size of the line on U axis
    noz_float u_size = msize[u_axis];

    // Intial size of the line on V axis
    noz_float v_size = msize[v_axis];

    // Loop as long as the last item fits on the current line
    for( ;u_last<c; u_last++) {
      child = GetChild(u_last);
      if(child->GetVisibility()==Visibility::Collapsed) continue;

      const Vector2& nmsize = child->GetMeasuredSize();
      if(u_size+nmsize[u_axis] > u_max) break;

      // Track maximum size on V axis within the line
      v_size = Math::Max(nmsize[v_axis], v_size);

      // Track total size on U axis for the line
      u_size += nmsize[u_axis];
    }

    // Size the intial item rectangle to include the maximum height of the line
    Rect item_rect;
    item_rect[u_axis] = nrect.GetMin()[u_axis];
    item_rect[v_axis] = v_offset + nrect.GetMin()[v_axis];
    item_rect[u_axis+2] = 0.0f;
    item_rect[v_axis+2] = v_size;

    // Arrange all items on the line
    for(noz_uint32 i=u_first; i != u_last; i++) {
      child = GetChild(i);
      if(child->GetVisibility()==Visibility::Collapsed) continue;

      // Adjust U axis max of the of the item
      item_rect[u_axis+2] = child->GetMeasuredSize()[u_axis];

      // Arrange the item
      child->Arrange(item_rect);

      // Move the U axis minimum up to the maximum
      item_rect[u_axis] += item_rect[u_axis+2];
    }

    // Adjust the offset by the line size
    v_offset += v_size;

    // Start next line on the last item
    i = u_last;
  }
}


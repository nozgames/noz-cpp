///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/TabItem.h>
#include "TabLayout.h"

using namespace noz;

TabLayout::TabLayout(void) {
  orientation_ = Orientation::Horizontal;
  overflow_ = TabOverflow::Wrap;
}

void TabLayout::SetOrientation(Orientation orientation) {
  if(orientation_ == orientation) return;
  orientation_ = orientation;
  InvalidateTransform();
}

Vector2 TabLayout::MeasureChildren (const Vector2& available_size) {
  Vector2 size;

  // The U axis is the axis on which content flows before wrapping
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis on which the panel grows when a wrap occurrs
  noz_int32 v_axis = !u_axis;

  // If not wrapping just stack all the tabs up
  if(overflow_!=TabOverflow::Wrap) {
    for(noz_uint32 i=0,c=GetChildCount();i<c;i++) {
      Node* child = GetChild(i);
      if(child->GetVisibility()==Visibility::Collapsed) continue;
      Vector2 msize = child->Measure(available_size);
      size[v_axis] = Math::Max(size[v_axis],msize[v_axis]);
      size[u_axis] += msize[u_axis];
    }    
    return size;
  }

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
    
void TabLayout::ArrangeChildren (const Rect& nrect) {
  // Nothing to do if no children.
  if(GetChildCount()==0) return;

  switch(overflow_) {
    case TabOverflow::Wrap: WrapArrange(nrect); break;
    case TabOverflow::Clip: ClipArrange(nrect); break;
    case TabOverflow::Overflow: OverflowArrange(nrect); break;
    case TabOverflow::Shrink: ShrinkArrange(nrect); break;
  }
}

void TabLayout::ShrinkArrange (const Rect& r) {
  // The U axis is the axis which content grows
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis which will have a fixed size
  noz_int32 v_axis = !u_axis;

  // Maximum size of u axis before content will wrap
  noz_float u_max = Math::Max(r[u_axis+2],0.0f);

  noz_float u_measured = GetMeasuredSize()[u_axis];

  if(u_measured <= u_max) {
    OverflowArrange(r);
    return;
  }

  noz_float u_overflow = u_measured - u_max;

  tracks_.clear();
  tracks_.reserve(GetChildCount());
  for(noz_uint32 i=0,c=GetChildCount();i<c;i++) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;
    Track t;
    t.size_ = child->GetMeasuredSize()[u_axis];
    t.first_child_ = i;
    tracks_.push_back(t);
  }

  while(u_overflow > 0.0f) {
    noz_float max_size = 0.0f;
    noz_float target_size = 0.0f;
    noz_int32 max_count = 0;

    for(noz_uint32 i=0,c=tracks_.size();i<c;i++) {
      Track& track = tracks_[i];
      if(track.size_ > max_size) {
        target_size = max_size;
        max_size = track.size_;
        max_count = 1;
      } else if (track.size_ == max_size) {
        max_count++;
      } else if (track.size_ > target_size) {
        target_size = track.size_;
      }
    }

    noz_float shrink = max_count * (max_size - target_size);
    shrink = Math::Min(shrink,u_overflow);
    shrink = Math::Ceil(shrink / max_count);

    for(noz_uint32 i=0,c=tracks_.size();i<c;i++) {
      Track& track = tracks_[i];
      if(track.size_ == max_size) {
        track.size_ -= shrink;
        u_overflow -= shrink;
      }
    }
  }

  // Arrange the children using calculated track sizes.
  Rect rect = r;
  rect[u_axis+2] = 0;

  for(noz_uint32 i=0,c=tracks_.size();i<c;i++) {
    Track& track = tracks_[i];
    Node* child = GetChild(i);
    rect[u_axis+2] = track.size_;
    GetChild(track.first_child_)->Arrange(rect);
    rect[u_axis] += track.size_;
  }
}

void TabLayout::OverflowArrange(const Rect& r) {
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
    noz_float u_size = child->GetMeasuredSize()[u_axis];
    rect[u_axis+2] = u_size;
    child->Arrange(rect);
    rect[u_axis] += u_size;
  } 
}

void TabLayout::ClipArrange(const Rect& r) {
  // The U axis is the axis which content grows
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis which will have a fixed size
  noz_int32 v_axis = !u_axis;
  
  // Maximum size of u axis before content will wrap
  noz_float u_max = r[u_axis] + r[u_axis+2];

  // Arrange all children that are not collapsed
  Rect rect = r;
  rect[u_axis+2] = 0;
  
  // Find the selected tab item.
  noz_uint32 selected = 0xFFFFFFFF;
  noz_float selected_size = 0.0f;
  for(noz_uint32 i=0,c=GetChildCount(); i<c; i++) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;
    if(child->IsTypeOf(typeof(TabItem)) && ((TabItem*)child)->IsSelected()) {
      selected = i;
      selected_size = child->GetMeasuredSize()[u_axis];
      break;
    }
  }

  bool clipped = false;
  for(noz_uint32 i=0,c=GetChildCount();i<c;i++) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;
    if(clipped && i!=selected) {child->ArrangeHide(); continue;}
    if(i==selected) selected_size = 0.0f;
    noz_float u_size = child->GetMeasuredSize()[u_axis];
    if(rect[u_axis] + u_size + selected_size > u_max) {
      clipped=true;
      child->ArrangeHide();
      continue;
    }
    rect[u_axis+2] = u_size;
    child->Arrange(rect);
    rect[u_axis] += u_size;
  } 
}      

void TabLayout::WrapArrange(const Rect& nrect) {
  // The U axis is the axis on which content flows normally before wrapping
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);

  // The V axis is the axis on which the panel grows when a wrap occurrs
  noz_int32 v_axis = !u_axis;

  // Maximum size of u axis before content will wrap
  noz_float u_max = nrect.GetSize()[u_axis];

  // Build the tracks..
  Track track;
  track.first_child_ = 0;
  track.last_child_ = 0;
  track.size_ = 0.0f;
  tracks_.clear();

  noz_uint32 selected = 0xFFFFFFFF;

  for(noz_uint32 i=0,c=GetChildCount(); i<c; i++) {
    Node* child = GetChild(i);
    if(child->GetVisibility()==Visibility::Collapsed) continue;

    // Find the selected tab item
    TabItem* tbitem = child->GetComponent<TabItem>();
    if(tbitem && tbitem->IsSelected()) selected = i;

    // If the child does not fit in the current track then create a new track for it.
    if(i != track.first_child_ && child->GetMeasuredSize()[u_axis] + track.size_ > u_max) {
      tracks_.push_back(track);
      track.first_child_ = i;
      track.size_ = 0.0f;
    }
    track.last_child_ = i;
    track.size_ += child->GetMeasuredSize()[u_axis];
  }

  // Add the last track if necessary
  if(tracks_.empty() || track.first_child_ != tracks_.back().first_child_) {
    tracks_.push_back(track);
  }

  // If only one track special case
  if(tracks_.size()==1) {
    ArrangeTrack(tracks_[0],Vector2(nrect.x,nrect.y),u_max);
    return;
  }

  while(1) {
    // Find the track with the minimum size.
    noz_uint32 min_track = 0;
    for(noz_uint32 i=1;i<tracks_.size();i++) {
      if(tracks_[i].size_ < tracks_[min_track].size_) min_track = i;
    }
    
    // If the smallest track is the first track then we are done
    if(min_track == 0) break;

    noz_assert(tracks_[min_track-1].first_child_ != tracks_[min_track-1].last_child_);

    noz_float prev_tab_size = GetChild(tracks_[min_track-1].last_child_)->GetMeasuredSize().x;
    noz_float adjusted_track_size = tracks_[min_track].size_ + prev_tab_size;

    if(adjusted_track_size > u_max) break;

    noz_float adjusted_prev_track_size = tracks_[min_track-1].size_ - prev_tab_size;
    if(adjusted_prev_track_size < tracks_[min_track].size_) break;

    tracks_[min_track].first_child_--;
    tracks_[min_track].size_ = adjusted_track_size;
    tracks_[min_track-1].last_child_--;
    tracks_[min_track-1].size_ = adjusted_prev_track_size;
  }

  Vector2 offset;
  offset[u_axis] = nrect.x;
  offset[v_axis] = nrect.y;
  Track* selected_track = nullptr;
  for(noz_uint32 i=0,c=tracks_.size();i<c;i++) {
    Track& track = tracks_[i];
    if(selected >= track.first_child_ && selected <= track.last_child_) {
      selected_track = &track;
      continue;
    }
    offset[v_axis] += ArrangeTrack(track, offset,u_max);
  }

  if(selected_track) ArrangeTrack(*selected_track, offset,u_max);
}


noz_float TabLayout::ArrangeTrack(const Track& track, const Vector2& offset, noz_float u_max) {
  noz_int32 u_axis = (orientation_ == Orientation::Vertical);
  noz_int32 v_axis = !u_axis;

  // Special case a single tab in the track
  if(track.first_child_ == track.last_child_) {
    const Vector2& msize = GetChild(track.first_child_)->GetMeasuredSize();
    Rect r;
    r.x = offset.x;
    r.y = offset.y;
    r[v_axis+2] = msize[v_axis];
    r[u_axis+2] = Math::Min(u_max,msize[u_axis]);
    GetChild(track.first_child_)->Arrange(r);
    return msize[v_axis];
  }

  // Calculate the max size on the v_axis
  noz_float v_size = 0.0f;
  for(noz_uint32 i=track.first_child_; i<= track.last_child_; i++) {
    v_size = Math::Max(v_size,GetChild(i)->GetMeasuredSize()[v_axis]);
  }

  noz_float avg_gap = 0.0f;
  if(tracks_.size()>1) {
    avg_gap = (noz_float)(noz_int32)((u_max - track.size_) / (track.last_child_-track.first_child_+1));
  }

  // Arrange the tabs
  Rect r(offset.x,offset.y,0.0f,0.0f);
  r[v_axis+2] = v_size;
  for(noz_uint32 i=track.first_child_; i<= track.last_child_; i++) {
    r[u_axis+2] = GetChild(i)->GetMeasuredSize()[u_axis] + avg_gap;
    GetChild(i)->Arrange(r);
    r[u_axis] += r[u_axis+2];
  }

  return v_size;
}


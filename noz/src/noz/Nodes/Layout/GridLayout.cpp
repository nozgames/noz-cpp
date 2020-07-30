///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/StringReader.h>
#include <noz/IO/BinaryWriter.h>
#include <noz/IO/BinaryReader.h>
#include <noz/Text/StringLexer.h>

#include "GridLayout.h"


using namespace noz;

GridLayout::GridLayout(void) {
  orientation_ = Orientation::Horizontal;
  tracks_invalid_ = true;
  cell_padding_ = 0.0f;
  cell_spacing_ = 0.0f;
}

Vector2 GridLayout::MeasureChildren (const Vector2& available_size) {
  // If the number of nodes in the grid exceeds the currently allocated tracks
  // then update the tracks.
  if(tracks_invalid_ || GetChildCount() > (noz_int32)(tracks_[0].size() * tracks_[1].size())) {
    UpdateTracks();
  }

  for(noz_uint32 a=0;a<2;a++) {
    for(noz_uint32 t=0;t<tracks_[a].size(); t++) {
      tracks_[a][t].measured_size_ = tracks_[a][t].min_size_ + cell_padding_ * 2.0f;
    }
  }

  for(noz_uint32 i=0, c=GetChildCount(); i<c && i<cells_.size(); i++) {
    GridTrack& row = tracks_[1][cells_[i].row];
    GridTrack& col = tracks_[0][cells_[i].col];

    Vector2 msize = GetChild(i)->Measure(available_size);
    row.measured_size_ = Math::Max(msize.y+cell_padding_*2.0f,row.measured_size_);
    col.measured_size_ = Math::Max(msize.x+cell_padding_*2.0f,col.measured_size_);
  }

  Vector2 size;
  for(noz_uint32 a=0;a<2;a++) {
    for(noz_uint32 t=0;t<tracks_[a].size(); t++) {
      GridTrack& track = tracks_[a][t];
      if(track.size_ == Float::NaN || track.size_is_ratio_) {
        size[a] += track.measured_size_;
      } else {
        size[a] += track.size_;
      }
    }
  }
  
  return size;
}
    
void GridLayout::ArrangeChildren (const Rect& nrect) {
  if(!IsAwake() || tracks_invalid_ || GetChildCount() > (noz_int32)(tracks_[0].size() * tracks_[1].size())) {
    return;
  }

  noz_uint32 i;
  noz_uint32 j;
  noz_uint32 c = GetChildCount();

  // Reset the actual size and offset for all tracks and total weights
  Vector2 total_weight;
  for(i=0; i<2; i++) {
    for(j=0; j<tracks_[i].size(); j++) {
      GridTrack& t = tracks_[i][j];
      t.actual_size_ = 0.0f;
      t.offset_ = 0.0f;
      if(t.size_is_ratio_) total_weight[i] += t.size_;
    }
  }

  // Calculate actual size for all auto-sized tracks 
 
  for(i=0; i<c && i<cells_.size(); i++) {
    GridTrack& row = tracks_[1][cells_[i].row];
    GridTrack& col = tracks_[0][cells_[i].col];

    Vector2 msize(col.measured_size_, row.measured_size_);

    // Update col
    if(col.size_ == Float::NaN) {
      if(msize.x > col.actual_size_) {
        col.actual_size_ = msize.x;
      }
    } else if(!col.size_is_ratio_) {
      col.actual_size_ = col.size_;
    }

    // Update row
    if(row.size_==Float::NaN) {
      if(msize.y > row.actual_size_) {
        row.actual_size_ = msize.y;
      }
    } else if(!row.size_is_ratio_) {
      row.actual_size_ = row.size_;
    }      
  }

  Vector2 total_fixed;
  for(i=0; i<2; i++) {
    for(j=0; j<tracks_[i].size(); j++) {
      GridTrack& t = tracks_[i][j];
      if(!t.size_is_ratio_) total_fixed[i] += t.actual_size_;
    }
  }

  // Update weights
  Vector2 total_non_fixed (
    Math::Max(0.0f,nrect.w - total_fixed.x),
    Math::Max(0.0f,nrect.h - total_fixed.y)
  );

  total_weight.x = total_weight.x > 0.0f ? 1.0f / total_weight.x : 0.0f;
  total_weight.y = total_weight.y > 0.0f ? 1.0f / total_weight.y : 0.0f;

  for(i=0; i<2; i++) {
    noz_float offset = 0;
    for(j=0; j<tracks_[i].size(); j++) {
      GridTrack& t = tracks_[i][j];
      if(t.size_is_ratio_) t.actual_size_ = total_non_fixed[i] * (t.size_ * total_weight[i]);
      t.offset_ = offset;
      offset += t.actual_size_;
    }
  }

  // Arrange the nodes

  for(i=0; i<c && i<cells_.size(); i++) {
    GridTrack& row = tracks_[1][cells_[i].row];
    GridTrack& col = tracks_[0][cells_[i].col];

    Rect crect(
      nrect.x+col.offset_+cell_padding_,
      nrect.y+row.offset_+cell_padding_,
      col.actual_size_-cell_padding_*2.0f,
      row.actual_size_-cell_padding_*2.0f);
    GetChild(i)->Arrange(crect);
  }
}

void GridLayout::SetOrientation(Orientation o) {
  orientation_ = o;
  tracks_invalid_ = true;
  InvalidateTransform();
}

void GridLayout::SetCellPadding (noz_float v) {
  if(cell_padding_ == v) return;
  cell_padding_ = v;
  InvalidateTransform();
}

void GridLayout::AddRow (noz_float height, bool height_is_ratio) {
  rows_.emplace_back();
  rows_.back().height_ = height;
  rows_.back().height_is_ratio_ = height_is_ratio;
  tracks_invalid_ = true;
  InvalidateTransform();
}

void GridLayout::AddColumn (noz_float width, bool width_is_ratio) {
  cols_.emplace_back();
  cols_.back().width_ = width;
  cols_.back().width_is_ratio_ = width_is_ratio;
  tracks_invalid_ = true;
  InvalidateTransform();
}

void GridLayout::SetRow (noz_int32 row, noz_float height, bool height_is_ratio) {
  if(row<0 || row>=(noz_int32)rows_.size()) return;
  rows_[row].height_ = height;
  rows_[row].height_is_ratio_ = height_is_ratio;
  tracks_invalid_ = true;
  InvalidateTransform();
}

void GridLayout::SetColumn (noz_int32 col, noz_float width, bool width_is_ratio) {
  if(col<0 || col>=(noz_int32)cols_.size()) return;
  cols_[col].width_ = width;
  cols_[col].width_is_ratio_ = width_is_ratio;
  tracks_invalid_ = true;
  InvalidateTransform();
}

void GridLayout::Clear(void) {
  rows_.clear();
  cols_.clear();
  InvalidateTransform();
}

void GridLayout::UpdateTracks (void) {
  if(!IsAwake()) return;

  noz_int32 node_count = GetChildCount();
  if(node_count == 0) return;
  
  // Deterine real row and column counts..
  noz_uint32 col_count = 0;
  noz_uint32 row_count = 0;
  if(orientation_==Orientation::Horizontal) {
    col_count = Math::Max((noz_uint32)1,cols_.size());
    row_count = Math::Max((noz_uint32)1,(node_count / col_count) + ((node_count % col_count)>0));
  } else {
    row_count = Math::Max((noz_uint32)1,rows_.size());
    col_count = Math::Max((noz_uint32)1,(node_count / row_count) + ((node_count % row_count)>0));
  }

  // Resize the tracks arrays to reflect the actual row and col counts
  tracks_[0].resize(col_count);
  tracks_[1].resize(row_count);

  // Copy the real row and column values
  for(noz_uint32 c=0;c<cols_.size();c++) {
    tracks_[0][c].size_ = cols_[c].width_;
    tracks_[0][c].size_is_ratio_ = cols_[c].width_is_ratio_;
    tracks_[0][c].min_size_ = cols_[c].min_width_;
  }
  for(noz_uint32 r=0;r<rows_.size();r++) {
    tracks_[1][r].size_ = rows_[r].height_;
    tracks_[1][r].size_is_ratio_ = rows_[r].height_is_ratio_;
    tracks_[1][r].min_size_ = rows_[r].min_height_;
  }

  // All tracks past the end of the defined rows and columns will copy
  // the values of the last real row or column
  noz_float size = cols_.size() ? cols_.back().width_ : Float::NaN;
  bool size_is_ratio = cols_.size() ? cols_.back().width_is_ratio_ : false;
  noz_float min_size = cols_.size() ? cols_.back().min_width_ : 0.0f;
  for(noz_uint32 cc=cols_.size(); cc<tracks_[0].size(); cc++) {
    tracks_[0][cc].size_ = size;
    tracks_[0][cc].size_is_ratio_ = size_is_ratio;
    tracks_[0][cc].min_size_ = min_size;
  }
  size = rows_.size() ? rows_.back().height_ : Float::NaN;
  size_is_ratio = rows_.size() ? rows_.back().height_is_ratio_ : false;
  min_size = rows_.size() ? rows_.back().min_height_: 0.0f;
  for(noz_uint32 rr=rows_.size(); rr<tracks_[1].size(); rr++) {
    tracks_[1][rr].size_ = size;
    tracks_[1][rr].size_ = size;
    tracks_[1][rr].min_size_ = min_size;
  }    

  // Populate the cells to make layout easier
  cells_.resize(node_count);
  
  if(orientation_==Orientation::Vertical) {
    noz_uint32 c=0;
    noz_uint32 r=0;
    for(noz_uint32 i=0; i<cells_.size(); i++) {
      cells_[i].row = r;
      cells_[i].col = c;
      if(++r>=tracks_[1].size()) {r=0;c++;}
    }      
  } else {
    noz_uint32 c=0;
    noz_uint32 r=0;
    for(noz_uint32 i=0; i<cells_.size(); i++) {
      cells_[i].row = r;
      cells_[i].col = c;
      if(++c>=tracks_[0].size()) {c=0;r++;}
    }      
  }

  tracks_invalid_ = false;
}

NOZ_TODO("IsTransformDependentOnChildren");
#if 0
bool GridLayout::IsInvalidIfChildInvalid (Node* child) const {
  noz_assert(child->GetVisualParent() == GetNode());

  // If the layout itself is invalid then dont bother checking the specific child.
  if(Layout::IsInvalidIfChildInvalid(child)) return true;

  // It is possible this method is being called while the track data is invalid.  Since 
  // we need the track data to determine if the child is in an auto-sized cell we need to
  // update it now.. However.. This method is a const method so we cant, but we are going
  // to be bad and cast away the const so we can.. Shh...
  if(tracks_invalid_) ((GridLayout*)this)->UpdateTracks();

  // Loop over the children and find the child node..
  Node* n = GetFirstChild(NodeTreeType::Visual);
  for(noz_uint32 i=0; n && i<cells_.size(); i++, n=n->GetNextSibling(NodeTreeType::Visual)) {
    // Skip until we find it.
    if(n != child) continue;

    // Get the row and column tracks for the child node
    const GridTrack& row = tracks_[1][cells_[i].row];
    const GridTrack& col = tracks_[0][cells_[i].col];
    
    // If either the row or column is auto then the grid is now invalid too
    return row.size_.IsAuto() || col.size_.IsAuto();
  }
  */

  return false;
}
#endif



#if 0

void foo (void) {
  node->

}



#endif
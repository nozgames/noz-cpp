///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "GridNode.h"

using namespace noz;
using namespace noz::Editor;


GridNode::GridNode (void) {
  size_ = 64.0f;
  color_ = Color::Black;
  subdivision_min_alpha_ = 0.25f;
  subdivision_max_alpha_ = 0.75f;
  scale_ = 1.0f;
  horizontal_ = true;
  vertical_ = true;
  horizontal_alignment_ = Alignment::Center;
  vertical_alignment_ = Alignment::Center;
  subdivisions_ = 8.0f;
}

GridNode::~GridNode(void) {
}

void GridNode::SetSubdivisions (noz_float subdivisions) {
  if(subdivisions == subdivisions_) return;
  subdivisions_ = subdivisions;
  if(subdivisions_ < 1.0f) subdivisions_= 0.0f;
}


bool GridNode::Render (RenderContext* rc, Rect& r) {
  if(color_.a==0) return true;

  noz_float a = 0.0f;
  noz_float gsize = size_ * scale_;
  noz_float gsize2 = 0.0f;
  if(subdivisions_ >= 1.0f) {    
    if(subdivision_min_alpha_ != subdivision_max_alpha_) {
      while(gsize > size_ * subdivisions_) gsize /= subdivisions_;
      while(gsize <size_) gsize *= subdivisions_;

      gsize2 = gsize / subdivisions_;
      a = (Math::Clamp(gsize2, (size_*subdivision_min_alpha_), (size_*subdivision_max_alpha_)) - (size_*subdivision_min_alpha_)) / (size_*(subdivision_max_alpha_-subdivision_min_alpha_));
    } else {
      gsize2 = gsize / subdivisions_;
      a = subdivision_min_alpha_;
    }
  }

  // Modulate the alpha for both grids to ensure that alpha is never fully 1.0.  This 
  // is done because grid2 will almost alwasy require alpha and to ensure that both
  // grid1 and grid2 remain in the same batch we need to maintain the same state
  // between them.  Having an alpha less than one ensures that alpha blending is enabled
  // for both grids.
  Color color1 = color_.ModulateAlpha(0.999f);
  Color color2 = color_.ModulateAlpha(Math::Min(0.999f,a));

  rc->PushMatrix();
  rc->MultiplyMatrix(GetLocalToViewport());  

  RenderGrid(rc, GetRectangle(), color1,gsize);

  if(a>0.0f) RenderGrid(rc, GetRectangle(), color2, gsize2);

  rc->PopMatrix();

  return true;
}

void GridNode::RenderGrid (RenderContext* rc, const Rect& r, Color color, noz_float size) {
  Vector2 v[2];

  if(vertical_) {
    noz_float xo = 0;

    switch(vertical_alignment_) {
      case Alignment::Min: xo = offset_.x; break;
      case Alignment::Max: {
        noz_float xoo = offset_.x + r.w;
        xo = xoo - ((noz_int32)(xoo / size)) * size;
        break;
      }
      case Alignment::Center: {
        noz_float xoo = offset_.x + r.w * 0.5f;
        xo = xoo - ((noz_int32)(xoo / size)) * size;
        break;
      }
    }

    noz_float y0 = r.y;
    noz_float y1 = r.y + r.h;
    for(noz_float x=r.x + xo; x < r.x + r.w; x += size) {
      v[0].set(x,y0);
      v[1].set(x,y1);
      rc->DrawDebugLine(v[0],v[1], color);
    }
  }

  if(horizontal_) {
    noz_float yo = 0;

    switch(horizontal_alignment_) {
      case Alignment::Min: yo = offset_.y; break;
      case Alignment::Max: {
        noz_float yoo = offset_.y + r.h;
        yo = yoo - ((noz_int32)(yoo / size)) * size;
        break;
      }
      case Alignment::Center: {
        noz_float yoo = offset_.y + r.h * 0.5f;
        yo = yoo - ((noz_int32)(yoo / size)) * size;
        break;
      }
    }

    noz_float x0 = r.x;
    noz_float x1 = r.x + r.w;
    for (noz_float y = r.y + yo; y < r.y + r.h; y += size) {
      v[0].set(x0, y);
      v[1].set(x1, y);
      rc->DrawDebugLine(v[0], v[1], color);
    }
  }
}



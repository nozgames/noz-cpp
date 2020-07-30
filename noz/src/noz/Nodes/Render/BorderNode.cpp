///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "BorderNode.h"

using namespace noz;

BorderNode::BorderNode(void) : mesh_(4,6) {
  color_ = Color::White;
  top_ = 1.0f;
  bottom_ = 1.0f;
  left_ = 1.0f;
  right_ = 1.0f;
  mesh_.SetCapacity(8,8);
}

void BorderNode::SetColor (Color c) {
  if(color_ == c) return;
  color_ = c;
  mesh_.SetColor(color_);
}

void BorderNode::SetSize (noz_float size) {
  if(top_ == size && bottom_ == size && left_ == size && right_ == size) return;
  top_ = bottom_ = left_ = right_ = size;
  Invalidate();
}

void BorderNode::SetTop (noz_float v) {
  if(top_==v) return;
  top_=v;
  Invalidate();
}

void BorderNode::SetBottom (noz_float v) {
  if(bottom_==v) return;
  bottom_=v;
  Invalidate();
}

void BorderNode::SetRight (noz_float v) {
  if(right_==v) return;
  right_=v;
  Invalidate();
}

void BorderNode::SetLeft (noz_float v) {
  if(left_==v) return;
  left_=v;
  Invalidate();
}

void BorderNode::UpdateMesh (const Rect& nr) {
  noz_float w_inv = 1.0f / nr.w;
  noz_float h_inv = 1.0f / nr.h;

  bool l = left_>0.0f;
  bool t = top_>0.0f;
  bool b = bottom_>0.0f;
  bool r = right_>0.0f;

  noz_int32 tlo;   /// Top Left Outside
  noz_int32 tli;   /// Top Left Inside
  noz_int32 blo;   /// Bottom Left Outside
  noz_int32 bli;   /// Bottom Left Inside
  noz_int32 tro;   /// Top Right Outside
  noz_int32 tri;   /// Top Right Inside
  noz_int32 bro;   /// Bottom Right Outside
  noz_int32 bri;   /// Bottom Right Inside

  mesh_.Clear();

  if(l || t) {
    tlo = mesh_.GetVerticies().size();
    mesh_.AddVertex(Vector2(nr.x,nr.y), Vector2(0.0f,0.0f), color_);

    tli = mesh_.GetVerticies().size();
    mesh_.AddVertex(Vector2(nr.x + left_,nr.y + top_), Vector2(left_ * w_inv,top_ * h_inv), color_);
  }

  if(l || b) {
    blo = mesh_.GetVerticies().size();
    mesh_.AddVertex(Vector2(nr.x,nr.y+nr.h), Vector2(0.0f,1.0f), color_);

    bli = mesh_.GetVerticies().size();
    mesh_.AddVertex(Vector2(nr.x + left_,nr.y+nr.h-bottom_), Vector2(left_ * w_inv,(nr.h - bottom_) * h_inv), color_);
  }

  if(r || b) {
    bro = mesh_.GetVerticies().size();
    mesh_.AddVertex(Vector2(nr.x+nr.w,nr.y+nr.h), Vector2(1.0f,1.0f), color_);

    bri = mesh_.GetVerticies().size();
    mesh_.AddVertex(Vector2(nr.x + nr.w - right_,nr.y+nr.h-bottom_), Vector2((nr.w - right_) * w_inv,(nr.h - bottom_) * h_inv), color_);
  }

  if(r || t) {
    tro = mesh_.GetVerticies().size();
    mesh_.AddVertex(Vector2(nr.x+nr.w,nr.y), Vector2(1.0f,0.0f), color_);

    tri = mesh_.GetVerticies().size();
    mesh_.AddVertex(Vector2(nr.x + nr.w - right_,nr.y + top_), Vector2((nr.w - right_) * w_inv,top_ * h_inv), color_);
  }

  if(l) {
    mesh_.AddTriangle(tlo,tli,blo);
    mesh_.AddTriangle(tli,bli,blo);
  }

  if(b) {
    mesh_.AddTriangle(bli,bri,bro);
    mesh_.AddTriangle(bli,bro,blo);
  }    

  if(r) {
    mesh_.AddTriangle(tri,tro,bro);
    mesh_.AddTriangle(tri,bro,bri);
  }    

  if(t) {
    mesh_.AddTriangle(tlo,tro,tri);
    mesh_.AddTriangle(tlo,tri,tli);
  } 
}

bool BorderNode::DrawMesh (RenderContext* rc) {
  if(color_.a == 0) return false;

  return rc->Draw(&mesh_);
}

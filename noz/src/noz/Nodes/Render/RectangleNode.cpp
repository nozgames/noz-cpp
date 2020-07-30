//////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RectangleNode.h"

using namespace noz;

RectangleNode::RectangleNode(void) : mesh_(4,6) {
  color_ = 0;
}

void RectangleNode::SetColor(Color color) {
  if(color_==color) return;
  color_ = color;
  mesh_.SetColor(color_);  
}
    
void RectangleNode::UpdateMesh (const Rect& r) {
  mesh_.Clear();

  mesh_.AddVertex(Vector2(r.x,r.y), Vector2::Zero, color_);
  mesh_.AddVertex(Vector2(r.x+r.w,r.y), Vector2::OneZero, color_);
  mesh_.AddVertex(Vector2(r.x+r.w,r.y+r.h), Vector2::One, color_);
  mesh_.AddVertex(Vector2(r.x,r.y+r.h), Vector2::ZeroOne, color_);

  mesh_.AddTriangle(0,1,2);
  mesh_.AddTriangle(0,2,3);
}

bool RectangleNode::DrawMesh (RenderContext* rc) {
  return rc->Draw(&mesh_);
}

//////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RenderNode.h"

using namespace noz;

RenderNode::RenderNode(void) {
  mesh_invalid_ = true;
}

bool RenderNode::Render(RenderContext* rc, Rect& render_rect) {
  if(mesh_invalid_) {
    UpdateMesh (GetRectangle());
    mesh_invalid_ = false;
  }

  // Render the mesh if there is at least one triangle
  bool rendered = false;

  rc->PushMatrix();
  rc->MultiplyMatrix(GetLocalToViewport());  
  rendered = DrawMesh(rc);
  rc->PopMatrix();

  return rendered;
}

void RenderNode::Invalidate (void) {
  if(mesh_invalid_) return;
  mesh_invalid_ = true;
  InvalidateTransform();
}

void RenderNode::Arrange(const Rect& r) {
  Rect old_rect = GetRectangle();
  Node::Arrange(r);
  if(GetRectangle() != old_rect) mesh_invalid_ = true;    
}

Vector2 RenderNode::MeasureChildren (const Vector2& a) {
  Vector2 size = Node::MeasureChildren(a);
  size = Math::Max(size,MeasureMesh(a));
  return size;
}

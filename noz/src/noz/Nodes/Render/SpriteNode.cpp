//////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "SpriteNode.h"

using namespace noz;

SpriteNode::SpriteNode(void) : mesh_(4,6) {
  stretch_ = Stretch::None;
}

void SpriteNode::SetSprite(Sprite* sprite) {
  if(sprite_ == sprite) return;
  sprite_ = sprite;
  Invalidate();
}

void SpriteNode::SetColor(Color color) {
  if(color_==color) return;
  color_ = color;
  mesh_.SetColor(color_);  
}
    
void SpriteNode::UpdateMesh (const Rect& rr) {
  mesh_.Clear();

  if(sprite_ == nullptr) return;

  Rect r = Math::StretchRect(stretch_, sprite_->GetSize(), rr);

  mesh_.SetImage(sprite_->GetImage());

  const Vector2& s = sprite_->GetS();
  const Vector2& t = sprite_->GetT();
  
  Color c[4] = {color_, color_, color_, color_};

  SpriteColors* colors = GetComponent<SpriteColors>();
  if(colors) {
    c[0] = colors->GetTopLeft();
    c[1] = colors->GetTopRight();
    c[2] = colors->GetBottomRight();
    c[3] = colors->GetBottomLeft();
  }

  mesh_.AddVertex(Vector2(r.x,r.y), s, c[0]);
  mesh_.AddVertex(Vector2(r.x+r.w,r.y), Vector2(t.x,s.y), c[1]);
  mesh_.AddVertex(Vector2(r.x+r.w,r.y+r.h), t, c[2]);
  mesh_.AddVertex(Vector2(r.x,r.y+r.h), Vector2(s.x,t.y), c[3]);

  mesh_.AddTriangle(0,1,2);
  mesh_.AddTriangle(0,2,3);
}

Vector2 SpriteNode::MeasureMesh (const Vector2& a) {
  if(nullptr == sprite_) return Vector2::Zero;
  switch(stretch_) {
    case Stretch::Uniform:
      return Math::StretchRect(stretch_, sprite_->GetSize(), Rect(0,0,a.x,a.y)).GetSize();

    default: break;      
  }

  return sprite_->GetSize();
}


bool SpriteNode::DrawMesh (RenderContext* rc) {
  if(color_.a==0) return false;
  return rc->Draw(&mesh_);
}

void SpriteNode::SetStretch (Stretch stretch) {
  if(stretch_ == stretch) return;
  stretch_ = stretch;
  Invalidate();
}

void SpriteColors::SetTopLeft (Color c) {
  if(colors_[0] == c) return;
  colors_[0] = c;
  if(GetNode() && GetNode()->IsTypeOf(typeof(SpriteNode))) ((SpriteNode*)GetNode())->Invalidate();
}

void SpriteColors::SetTopRight (Color c) {
  if(colors_[1] == c) return;
  colors_[1] = c;
  if(GetNode() && GetNode()->IsTypeOf(typeof(SpriteNode))) ((SpriteNode*)GetNode())->Invalidate();
}

void SpriteColors::SetBottomRight (Color c) {
  if(colors_[2] == c) return;
  colors_[2] = c;
  if(GetNode() && GetNode()->IsTypeOf(typeof(SpriteNode))) ((SpriteNode*)GetNode())->Invalidate();
}

void SpriteColors::SetBottomLeft (Color c) {
  if(colors_[3] == c) return;
  colors_[3] = c;
  if(GetNode() && GetNode()->IsTypeOf(typeof(SpriteNode))) ((SpriteNode*)GetNode())->Invalidate();
}

void SpriteColors::OnDetach (Node* node) {
  if(node->IsTypeOf(typeof(SpriteNode))) ((SpriteNode*)node)->Invalidate();
}

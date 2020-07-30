///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationTimeline.h"
#include "AnimationView.h"

using namespace noz;
using namespace noz::Editor;


AnimationTimeline::AnimationTimeline (void) {
  spacing_ = 100.0f;
  text_color_ = Color::White;
  margin_ = 0.0f;
  font_ = Font::GetDefaultFont();
  zoom_ = 0.0f;
  time_ = 0.0f;
  SetZoom(1.0f);
  time_offset_ = 0.0f;
  SetClipChildren(true);
}

void AnimationTimeline::SetZoom (noz_float z) {
  if(z==zoom_) return;
  zoom_ = z;

  if(zoom_ < 0.35f) {
    label_interval_ = 5.0f;
  } else if(zoom_ < 0.5f) {
    label_interval_ = 2.0f;
  } else if(zoom_ > 7.5f) {
    label_interval_ = 0.1f;
  } else if(zoom_ > 3.5f) {
    label_interval_ = 0.2f;
  } else if(zoom_ > 1.5f) {
    label_interval_= 0.5f;
  } else {
    label_interval_ = 1.0f;
  }

  snap_time_ = label_interval_ / 10.0f;

  InvalidateTransform();
}

void AnimationTimeline::SetTimeOffset (noz_float o) {
  if(o==time_offset_) return;
  time_offset_ = o;
  InvalidateTransform();
}

void AnimationTimeline::SetFont (Font* font) {
  if(font == nullptr) font = Font::GetDefaultFont();
  if(font == font_) return;
  font_ = font;
  InvalidateTransform();
}

void AnimationTimeline::RenderOverride (RenderContext* rc) {
  UINode::RenderOverride(rc);

  rc->PushMatrix();
  rc->MultiplyMatrix(GetLocalToViewport());    
  rc->Draw(&mesh_);
  rc->Draw(&label_mesh_);
  rc->Draw(&time_mesh_);
  rc->PopMatrix();
}

void AnimationTimeline::ArrangeChildren(const Rect& r) {
  // Default arrange.
  UINode::ArrangeChildren(r);

  // Background..
  mesh_.Clear();
  mesh_.AddVertex(Vector2(r.x,r.y), Vector2::Zero, background_color_);
  mesh_.AddVertex(Vector2(r.x+r.w,r.y), Vector2::Zero, background_color_);
  mesh_.AddVertex(Vector2(r.x+r.w,r.y+r.h), Vector2::Zero, background_color_);
  mesh_.AddVertex(Vector2(r.x,r.y+r.h), Vector2::Zero, background_color_);
  mesh_.AddTriangle(0,1,2);
  mesh_.AddTriangle(0,2,3);

  // Rebuild the text mesh based on the visible numbers
  label_mesh_.Clear();
  label_mesh_.SetImage(font_->GetImage());

  noz_float xs = GetPixelsPerSecond() * label_interval_;
  noz_float x1 = r.x + margin_ - GetPixelsPerSecond() * time_offset_;
  noz_float x2 = r.x + r.w;     
  noz_float t = 0.0f;
  noz_float y = r.y+r.h*0.375f-font_->GetHeight()*0.5f;
  noz_float xxs = xs / 10.0f;
  for(noz_float x=x1; x<=x2; x += xs, t+=label_interval_) {
    noz_float xx = x;
    if(x >= r.x) {
      String s = String::Format("%3.1f", t);
      if(s[s.GetLength()-1]=='0') s = s.Substring(0,s.GetLength()-2);
      AddTimeText(x,y, s.ToCString());

      noz_int32 v = mesh_.GetVerticies().size();
      mesh_.AddTriangle(v,v+1,v+2);
      mesh_.AddTriangle(v,v+2,v+3);
      mesh_.AddVertex(Vector2(xx,r.y+r.h*0.75f), Vector2::Zero, text_color_);
      mesh_.AddVertex(Vector2(xx+1,r.y+r.h*0.75f), Vector2::Zero, text_color_);
      mesh_.AddVertex(Vector2(xx+1,r.y+r.h), Vector2::Zero, text_color_);    
      mesh_.AddVertex(Vector2(xx,r.y+r.h), Vector2::Zero, text_color_);    
    }

    xx+=xxs;

    for(noz_uint32 i=1;i<10;i++,xx += xxs) {
      if(x < r.x) continue;

      noz_int32 v = mesh_.GetVerticies().size();
      mesh_.AddTriangle(v,v+1,v+2);
      mesh_.AddTriangle(v,v+2,v+3);
      mesh_.AddVertex(Vector2(xx,r.y+r.h*0.75f), Vector2::Zero, line_color_);
      mesh_.AddVertex(Vector2(xx+1,r.y+r.h*0.75f), Vector2::Zero, line_color_);
      mesh_.AddVertex(Vector2(xx+1,r.y+r.h), Vector2::Zero, line_color_);    
      mesh_.AddVertex(Vector2(xx,r.y+r.h), Vector2::Zero, line_color_);    
    }
  }

  if(time_marker_) {
    noz_float ml = x1 + GetPixelsPerSecond() * time_ - time_marker_->GetSize().x * 0.5f;
    noz_float mr = ml + time_marker_->GetSize().x;

    time_mesh_.Clear();
    time_mesh_.SetImage(time_marker_->GetImage());
    time_mesh_.AddQuad(
      Vector2(ml,r.y+r.h*0.75f),
      Vector2(mr,r.y+r.h),
      time_marker_->GetS(),
      time_marker_->GetT(),
      time_marker_color_
    );
  }
}

void AnimationTimeline::AddTimeText (noz_float x, noz_float y, const char* text) {
  noz_assert(text);
  noz_assert(*text);
  noz_assert(font_);

  // Length of text..
  noz_int32 text_len = String::GetLength(text);

  // Calculate the size of the text
  noz_float width = 0.0f;
  for(noz_int32 i=1; i<text_len; i++) {    
    width += font_->GetAdvanceX(text[i-1],text[i]);
  }
  width += (font_->GetGlyph(text[text_len-1])->w + font_->GetGlyph(text[text_len-1])->ox);

  // Increase the text mesh size to accomidate the new text.
  label_mesh_.SetCapacity(label_mesh_.GetVerticies().size() + text_len * 4, label_mesh_.GetTriangles().size() + text_len * 2);

  // Center the text.
  x -= (width * 0.5f);
  x ++;

  for(noz_int32 i=0;i<text_len;i++) {
    const Font::Glyph* glyph = font_->GetGlyph(text[i]);
    noz_assert(glyph);

    if(i>0) x += font_->GetAdvanceX(text[i-1],text[i]);

    // Calculate four corners for glyph quad
    noz_float l = x + glyph->ox;
    noz_float t = y + font_->GetAscent() - glyph->oy;
    noz_float r = l + glyph->w;
    noz_float b = t + glyph->h;

    // Add verts..
    noz_int32 base = label_mesh_.GetVerticies().size();
    label_mesh_.AddVertex(Vector2(l,t),Vector2(glyph->s.x,glyph->s.y), text_color_);
    label_mesh_.AddVertex(Vector2(r,t),Vector2(glyph->t.x,glyph->s.y), text_color_);
    label_mesh_.AddVertex(Vector2(r,b),Vector2(glyph->t.x,glyph->t.y), text_color_);
    label_mesh_.AddVertex(Vector2(l,b),Vector2(glyph->s.x,glyph->t.y), text_color_);

    // Add tris..
    label_mesh_.AddTriangle(base + 0, base + 1, base + 3);
    label_mesh_.AddTriangle(base + 1, base + 2, base + 3);
  }
}

void AnimationTimeline::OnMouseDown (SystemEvent* e) {
  UINode::OnMouseDown(e);

  SetTimeFromPosition(e->GetPosition());

  SetCapture();
}

void AnimationTimeline::OnMouseOver (SystemEvent* e) {
  UINode::OnMouseOver(e);

  if(HasCapture()) {
    SetTimeFromPosition(e->GetPosition());
  }
}

void AnimationTimeline::OnMouseUp (SystemEvent* e) {
  UINode::OnMouseUp(e);

  if(HasCapture()) {
    SetTimeFromPosition(e->GetPosition());
  }
}

void AnimationTimeline::SetTime (noz_float t) {
  t = Math::Max(t,0.0f);
  if(t==time_) return;
  time_ = t;
  ValueChanged(this);
  InvalidateTransform();
}

void AnimationTimeline::SetTimeFromPosition (const Vector2& position) {
  if(nullptr==animation_view_) return;

  Vector2 pos = WindowToLocal(position);

  noz_float left_offset = pos.x - (GetRectangle().x + margin_ - time_offset_ * GetPixelsPerSecond());
  noz_float new_time = (left_offset / GetPixelsPerSecond());
  if(snap_time_ > 0.0f) {
    new_time = ((noz_float)(noz_int32)((new_time / snap_time_) + 0.5f)) * snap_time_;
  }

  SetTime(new_time);
}


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "EditableTextNode.h"

using namespace noz;


EditableTextNode::EditableTextNode(void) : selection_mesh_(4,6) {
  selection_start_ = 0;
  selection_end_ = 0;
  caret_color_ = 0;
  selection_color_ = 0;
}

EditableTextNode::~EditableTextNode(void) {
}

bool EditableTextNode::DrawMesh(RenderContext* rc) { 
  // Render the text..
  bool rendered = TextNode::DrawMesh(rc);

  UpdateSelectionRenderState();

  // Draw the selection on top of the text
  if(!selection_mesh_.GetTriangles().empty()) {
    if(HasSelection()) {
      if(selection_color_.a) rendered |= rc->Draw(&selection_mesh_);
    } else if(caret_color_.a) {
      rendered |= rc->Draw(&selection_mesh_);
    }
  }

  return rendered;
}

Vector2 EditableTextNode::GetPositionFromCaret (void) {  
  return characters_[Math::Min(characters_.size()-1,(noz_uint32)selection_end_)].bounds_.GetMin();
}

void EditableTextNode::Select(noz_int32 sel) {
  SetSelectionStart(sel);
  SetSelectionEnd(sel);
}  

void EditableTextNode::Select(noz_int32 ss, noz_int32 se) {
  SetSelectionStart(ss);
  SetSelectionEnd(se);
}  

void EditableTextNode::SetSelectionStart(noz_int32 sel) {
  selection_start_ = Math::Clamp(sel,0,GetText().GetLength());
}

void EditableTextNode::SetSelectionEnd(noz_int32 sel) {
  selection_end_ = Math::Clamp(sel,0,GetText().GetLength());
}

void EditableTextNode::SetSelectionColor(Color col) {
  if(selection_color_==col) return;
  selection_color_ = col;
  if(HasSelection()) selection_mesh_.SetColor(col);
}

void EditableTextNode::SetCaretColor(Color col) {
  if(caret_color_ == col) return;
  caret_color_ = col;
  if(!HasSelection()) selection_mesh_.SetColor(col);
}

void EditableTextNode::ReplaceText(const char* text) {
  noz_int32 ss = GetSelectionMin();
  noz_int32 se = GetSelectionMax();

  StringBuilder sb(GetText().Substring(0,ss));
  sb.Append(text);
  if(se+1<=GetText().GetLength()) sb.Append(GetText().Substring(se+1));
  SetText(sb.ToString());
  Select(ss+String::GetLength(text));
}

void EditableTextNode::ReplaceText(char c) {
  noz_int32 ss = GetSelectionMin();
  noz_int32 se = GetSelectionMax();

  StringBuilder sb(GetText().Substring(0,ss));
  sb.Append(c);
  if(se+1<=GetText().GetLength()) sb.Append(GetText().Substring(se+1));
  SetText(sb.ToString());
  Select(ss+1);
}

void EditableTextNode::UpdateMesh (const Rect& r) {
  TextNode::UpdateMesh(r);

  // Ensure the selection is clamped to the text
  selection_start_ = Math::Clamp(selection_start_,0,GetText().GetLength());
  selection_end_ = Math::Clamp(selection_end_,0,GetText().GetLength());

  // Update the selection render state now that we have room.
  UpdateSelectionRenderState();
}

void EditableTextNode::UpdateSelectionRenderState(void) {
  // Allocate a new render mesh if necessary.
  noz_uint32 selection_vert_count = lines_.size() * 4;
  noz_uint32 selection_tri_count = lines_.size() * 2;

  selection_mesh_.Clear();

  // Normalize the selection start and end to ensure start is before end.
  noz_int32 ss = GetSelectionMin();
  noz_int32 se = GetSelectionMax();

  // If there is a selection render the selection on top.
  if(HasSelection()) {
    noz_int32 line = characters_[ss].line_;
    noz_int32 line_s = ss;
    noz_int32 line_e = ss;
    while(line_s <= se) {
      noz_int32 line = characters_[line_s].line_;
      for(line_e=line_s; line_e <= se && characters_[line_e].line_ == line; line_e++);
      line_e--;

      const Rect& cs = characters_[line_s].bounds_;
      const Rect& ce = characters_[line_e].bounds_;
      noz_float l = cs.x;
      noz_float r = ce.x + ce.w;
      noz_float t = cs.y;
      noz_float b = cs.y + cs.h;

      noz_int32 base = selection_mesh_.GetVerticies().size();
      selection_mesh_.AddVertex(Vector2(l,t),Vector2(0,0),selection_color_);
      selection_mesh_.AddVertex(Vector2(r,t),Vector2(1,0),selection_color_);
      selection_mesh_.AddVertex(Vector2(r,b),Vector2(1,1),selection_color_);          
      selection_mesh_.AddVertex(Vector2(l,b),Vector2(0,1),selection_color_);
      selection_mesh_.AddTriangle(base + 0, base + 1, base + 2);
      selection_mesh_.AddTriangle(base + 0, base + 2, base + 3);

      line_s = line_e + 1;
    }

  } else {
    Character& c = characters_[ss];

    noz_float l = c.bounds_.x;
    noz_float r = l + 1.0f;
    noz_float t = c.bounds_.y;
    noz_float b = t + c.bounds_.h;

    selection_mesh_.AddVertex(Vector2(l,t),Vector2(0,0),caret_color_);
    selection_mesh_.AddVertex(Vector2(r,t),Vector2(1,0),caret_color_);
    selection_mesh_.AddVertex(Vector2(r,b),Vector2(1,1),caret_color_);          
    selection_mesh_.AddVertex(Vector2(l,b),Vector2(0,1),caret_color_);
    selection_mesh_.AddTriangle(0,1,2);
    selection_mesh_.AddTriangle(0,2,3);
  }
}

void EditableTextNode::OnSetText (void) {  
}

Vector2 EditableTextNode::MeasureMesh (const Vector2& a) {
  return TextNode::MeasureMesh(a) + Vector2(2.0f,0.0f);
}

String EditableTextNode::GetSelectedText (void) const {
  noz_int32 ss = GetSelectionMin();
  noz_int32 se = GetSelectionMax();
  if(ss==se) return String::Empty;
  return GetText().Substring(ss,se-ss+1);
}

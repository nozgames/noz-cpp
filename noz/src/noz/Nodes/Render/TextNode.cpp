///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "TextNode.h"

using namespace noz;

TextNode::TextNode(void) {
  font_ = Font::GetDefaultFont();
  horizontal_alignment_ = Alignment::Min;
  horizontal_overflow_ = TextOverflowHorizontal::Clip;
  vertical_alignment_ = Alignment::Min;
  vertical_overflow_ = TextOverflowVertical::Clip;
  color_ = Color::White;
  measure_dirty_ = true;
}

TextNode::~TextNode(void) {
}

void TextNode::SetHorizontalAlignment(Alignment align) {
  // Prevent double setting
  if(horizontal_alignment_ == align) return;

  // Set new value
  horizontal_alignment_ = align;

  Invalidate();
}

void TextNode::SetVerticalAlignment(Alignment align) {
  // Prevent double setting
  if(vertical_alignment_ == align) return;

  // Set new value
  vertical_alignment_ = align;

  Invalidate();
}

void TextNode::SetHorizontalOverflow(TextOverflowHorizontal overflow) {
  horizontal_overflow_ = overflow;
}

void TextNode::SetVerticalOverflow(TextOverflowVertical overflow) {
  vertical_overflow_ = overflow;
}

void TextNode::SetFont(Font* font) {
  // Prevent double setting font
  if(font_ == font) return;

  // Set new font.
  font_ = font;  

  // If no font was set use the default font.
  if(font_ == nullptr) font_ = AssetManager::LoadAsset<Font>("fonts/ArialBlack");

  // Mark the renderer as dirty
  measure_dirty_ = true;

  // Invalidate our render state.
  Invalidate();

  // Invalidate the parent node to ensure everything is recalculated
  InvalidateTransform();
}

void TextNode::SetText(const char* text) {
  // Ensure the text actually changed
  if(!text_.CompareTo(text)) return;

  // Set new text
  text_ = text;

  // Set dirty flags
  measure_dirty_ = true;  

  Invalidate();

  // Invalidate the parent node to ensure everything is recalculated
  InvalidateTransform();
}

Vector2 TextNode::MeasureMesh(const Vector2& avail) {
  // If the measurement hasnt changed return the cached value
  if(!measure_dirty_) return measure_size_;

  // Flag the measure as no longer dirty
  measure_dirty_ = false;
  measure_size_.clear();

  // No font no size.
  if(font_ == nullptr) return measure_size_;

  noz_float height = font_->GetHeight();  

  measure_size_.y = height - font_->GetLineSpacing();

  // Update the measure size
  noz_float line_w = 0;
  noz_float line_x = 0;
  noz_int32 line_count = 1;
  char char_last = 0;
  for(noz_int32 i=0; i<text_.GetLength(); i++) {
    // Handle new line.
    if(text_[i] == '\n') {
      measure_size_.x = Math::Max(line_w,measure_size_.x);
      measure_size_.y += height;
      line_w = 0.0f;
      line_x = 0.0f;
      line_count++;
      char_last = 0;
      continue;
    }

    // Skip the character if there is no glyph for it.
    const Font::Glyph* glyph = font_->GetGlyph(text_[i]);
    if(nullptr == glyph) continue;

    // Calculate the advance from the previous character to the 
    // current character.
    line_x += font_->GetAdvanceX(char_last,text_[i]);

    // Adjust line width
    line_w = line_x + glyph->ox + glyph->w;

    // Advance to the next character position
    char_last = text_[i];
  }

  if(line_w) {
    measure_size_.x = Math::Max(line_w,measure_size_.x);
  }

  return measure_size_;
}

void TextNode::SetColor (Color color) {
  if(color_ == color) return;
  color_ = color;
  mesh_.SetColor(color);
}

void TextNode::UpdateMesh(const Rect& nrect) {
  if(nullptr==font_) {
    mesh_.Clear();
    return;
  }

  // Cache ascent and height
  noz_float height = font_->GetHeight();
  noz_float line_spacing = font_->GetLineSpacing();

  // If horizontal overflow is ellipsis we need to calculate the ellipsis size
  noz_float ellipsis_width = 0.0f;
  const Font::Glyph* dot_glyph = nullptr;
  noz_float dot_advance = 0.0f;
  if(horizontal_overflow_ == TextOverflowHorizontal::Ellipsis) {
    dot_glyph = font_->GetGlyph('.');
    dot_advance = font_->GetAdvanceX('.','.');
    if(nullptr != dot_glyph) {
      ellipsis_width += dot_advance * 2.0f;
      ellipsis_width += (dot_glyph->ox + dot_glyph->w);
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // PASS1: Calculate the number of valid characters and lines in the text
  //////////////////////////////////////////////////////////////////////////////

  noz_int32 char_count = 0;
  noz_int32 line_count = 1;
  noz_int32 i;
  for(i=0; i<text_.GetLength(); i++) {
    // Special case new line..
    if(text_[i] == '\n') {
      line_count++;
      continue;
    }

    // Skip characters with no glyph in the font.
    const Font::Glyph* glyph = font_->GetGlyph(text_[i]);
    if(glyph==nullptr) continue;

    // Include this character.
    char_count++;
  }

  // Add a character for each line for the EOL
  char_count += line_count;

  // If horizontal overflow is an ellipsis then add additional characters to account for 
  // possible dots on every line (two per line since we know at least one character will be removed
  // and replaced with the ellipsis).
  if(horizontal_overflow_==TextOverflowHorizontal::Ellipsis) char_count += (2*line_count);

  //////////////////////////////////////////////////////////////////////////////
  // PASS2: Populate the Characters and handle horizontal clipping
  //////////////////////////////////////////////////////////////////////////////
  noz_int32 line = 0;
  noz_float line_x = 0.0f;
  noz_float line_y = 0.0f;
  bool line_skip = false;

  characters_.reserve(char_count);
  characters_.clear();

  // Ensure a single line will fit if clipping vertically..
  if(vertical_overflow_ == TextOverflowVertical::Clip && height - line_spacing > nrect.h) {
    i = text_.GetLength();
  } else {
    i = 0;
  }

  char char_last = 0;
  noz_uint32 char_render_count = 0;
  for(; i<text_.GetLength(); i++) {
    // Special case newline.
    if(text_[i] == '\n') {
      // Add EOL character
      Character eol;
      eol.glyph_ = nullptr;
      eol.bounds_.x = line_x + font_->GetAdvanceX(char_last,' ');
      eol.line_ = line;
      eol.char_index_ = i;
      characters_.push_back(eol);

      line_x = 0.0f;
      line_y += height;

      // We are done if already overflowed.
      if(vertical_overflow_ == TextOverflowVertical::Clip && line_y > nrect.h) {
        break;
      }

      line++;

      line_skip = false;
      char_last = 0;
      continue;
    }

    // Are we skipping the remaining characters on the line?
    if(line_skip) continue;

    // Get character glyph
    const Font::Glyph* glyph = font_->GetGlyph(text_[i]);

    // Skip the glyph if it cant be renderered.
    if(glyph==nullptr) continue;

    // Calculate the advance from the previous character to the current.  We wait
    // to do this now because the line may have changed or a missing character
    // may have been found.
    noz_float advance_x = font_->GetAdvanceX(char_last,text_[i]);

    // Did the glyph overflow?
    if(horizontal_overflow_ != TextOverflowHorizontal::Overflow) {
      if(line_x + advance_x + glyph->ox + glyph->w > nrect.w) {
        switch(horizontal_overflow_) {
          case TextOverflowHorizontal::Wrap: {
            noz_uint32 cc=0;
            for(cc=characters_.size(); cc>0 && characters_[cc-1].line_==line; cc--) {
              char c = text_[characters_[cc-1].char_index_];
              if(c==' ') break;
            }

            cc--;

            if(characters_[cc].line_==line && text_[characters_[cc].char_index_]==' ') {
              // Add EOL character
              Character eol;
              eol.glyph_ = nullptr;
              eol.bounds_.x = line_x + font_->GetAdvanceX(char_last,' ');
              eol.line_ = line;
              eol.char_index_ = i;
              characters_.push_back(eol);

              line_x = 0.0f;
              line_y += height;

              // We are done if already overflowed.
              if(vertical_overflow_ == TextOverflowVertical::Clip && line_y > nrect.h) {
                break;
              }

              line++;
              i = cc-1;
              while(characters_.size() > cc) characters_.pop_back();

              line_skip = false;
              char_last = 0;
              continue;
            }
            break;
          }            

          case TextOverflowHorizontal::Clip:
            // Skip all remaining characters on the line
            line_skip = true;
            continue;

          case TextOverflowHorizontal::Ellipsis: {
            // Skip remainder of line.
            line_skip = true;

            // If the ellipsis doesnt even fit all by itself then just render a blank line
            if(ellipsis_width > nrect.w) {
              while(!characters_.empty() && characters_.back().line_ == line) {
                characters_.pop_back();
              }
              continue;
            }

            // Pop characters until the ellipsis fits
            while(!characters_.empty() && characters_.back().line_ == line) {
              line_x = characters_.back().bounds_.x;
              line_x += font_->GetAdvanceX(text_[characters_.back().char_index_],'.');
              if(line_x + ellipsis_width < nrect.w) break;
              characters_.pop_back();
            }

            // If no characters left on the line then render only the ellipsis
            if(characters_.empty() || characters_.back().line_ != line) {
              line_x = 0.0f;
            }

            // Add the ellipsis dots..
            for(noz_int32 i=0;i<3;i++) {
              Character c;
              c.glyph_ = dot_glyph;
              c.line_ = line;
              c.bounds_.x = line_x;
              c.bounds_.w = glyph->w;
              c.char_index_ = -1;
              line_x += dot_advance;
              characters_.push_back(c);
            }              

            continue;
          }

          default:
            noz_assert(false);
            break;
        }
      }
    }

    // Remember the last character added
    char_last = text_[i];

    // Add the advance since we are committed to this glyph now
    line_x += advance_x;

    // Add a character
    Character c;
    c.glyph_ = glyph;
    c.line_ = line;
    c.bounds_.x = line_x;
    c.bounds_.w = glyph->w;
    c.char_index_ = i;
    characters_.push_back(c);

    // If the character has a width then mark it to render.
    if(c.bounds_.w > 0.0f) char_render_count++;
  }

  // Add an EOL if not already added
  if(characters_.empty() || characters_.back().glyph_) {
    Character eol;
    eol.glyph_ = nullptr;
    eol.line_ = line;
    eol.bounds_.x = line_x + font_->GetAdvanceX(char_last, ' ');
    characters_.push_back(eol);
  }

  //////////////////////////////////////////////////////////////////////////////
  // PASS3: Populate Lines 
  //////////////////////////////////////////////////////////////////////////////

  // Resize the lines.
  lines_.resize(line+1);

  for(noz_uint32 l=0, c=0; l<lines_.size(); l++, c++) {
    Line& line = lines_[l];

    line.bounds_.x = 0.0f;    

    // Skip empty lines..
    if(characters_[c].line_ != l) {
      line.bounds_.y = line_y;
      line.bounds_.h = l * height;
      line.bounds_.w = 0.0f;
      line.char_min_ = -1;
      line.char_max_ = -1;
      continue;
    }

    // Set first character in the line.      
    line.char_min_ = c;

    // Find the last character in the line
    for(; c+1 < characters_.size() && characters_[c+1].line_ == l; c++);

    // Set the last character in the line 
    line.char_max_ = c;

    // Set line bounds.
    line.bounds_.w = characters_[c].bounds_.x;
    line.bounds_.y = l * height;
    line.bounds_.h = height;
  }

  // Remove the line spacing from the height of the last line.
  Line& line_last = lines_[lines_.size()-1];
  line_last.bounds_.h -= line_spacing;

  switch(horizontal_alignment_) {
    case Alignment::Max: 
      for(noz_uint32 li=0;li<lines_.size();li++) {
        Line& line = lines_[li];
        line.bounds_.x += (nrect.w - line.bounds_.w);
      }
      break;

    case Alignment::Center: 
      for(noz_uint32 li=0;li<lines_.size();li++) {
        Line& line = lines_[li];
        line.bounds_.x += (nrect.w - line.bounds_.w) * 0.5f;
      }
      break;
      
    default: break;
  }

  switch(vertical_alignment_) {
    case Alignment::Max: {
      noz_float offset = (nrect.h - (line_last.bounds_.y + line_last.bounds_.h));
      for(noz_uint32 li=0;li<lines_.size();li++) {
        Line& line = lines_[li];
        line.bounds_.y += offset;
      }
      break;
    }

    case Alignment::Center: {
      noz_float offset = (nrect.h - (line_last.bounds_.y + line_last.bounds_.h)) * 0.5f;
      for(noz_uint32 li=0;li<lines_.size();li++) {
        Line& line = lines_[li];
        line.bounds_.y += offset;
      }
      break;
    }
    
    default:break;
  }

  //////////////////////////////////////////////////////////////////////////////
  // PASS4: Populate verts_ and tris_
  //////////////////////////////////////////////////////////////////////////////

  // Allocate a new mesh if there was no mesh or the counts change.
  noz_uint32 char_vertex_count = char_render_count * 4;
  noz_uint32 char_triangle_count = char_render_count * 2;
  mesh_.SetCapacity(char_vertex_count, char_triangle_count);
  
  // Set the mesh texture using the font texture.
  mesh_.Clear();
  mesh_.SetImage(font_->GetImage());

  for(noz_uint32 l=0; l<lines_.size(); l++) {
    // Cache render line reference
    Line& line = lines_[l];

    // Skip empty lines.
    if(line.char_min_==-1) continue;

    // Add the parent rectangle into the line bounds.
    line.bounds_.x += nrect.x;
    line.bounds_.y += nrect.y;

    // Process each charcter in the line
    for(noz_uint32 i=line.char_min_; i<=line.char_max_; i++) {
      Character& c = characters_[i];
            
      // Adjust the character bounds to be relative to the line
      c.bounds_.x += line.bounds_.x;
      c.bounds_.y += line.bounds_.y;
      c.bounds_.h = line.bounds_.h;

      if(c.glyph_) {
        // Set glyph specific bounds values
        c.bounds_.x += c.glyph_->ox;
        c.bounds_.w = c.glyph_->w;

        // Calculate four corners for glyph quad
        noz_float l = c.bounds_.x;
        noz_float t = c.bounds_.y + font_->GetAscent() - line_spacing - c.glyph_->oy;
        noz_float r = l + c.glyph_->w;
        noz_float b = t + c.glyph_->h;

        // Add verts..
        noz_int32 base = mesh_.GetVerticies().size();
        mesh_.AddVertex(Vector2(l,t),Vector2(c.glyph_->s.x,c.glyph_->s.y), color_);
        mesh_.AddVertex(Vector2(r,t),Vector2(c.glyph_->t.x,c.glyph_->s.y), color_);
        mesh_.AddVertex(Vector2(r,b),Vector2(c.glyph_->t.x,c.glyph_->t.y), color_);
        mesh_.AddVertex(Vector2(l,b),Vector2(c.glyph_->s.x,c.glyph_->t.y), color_);

        // Add tris..
        mesh_.AddTriangle(base + 0, base + 1, base + 3);
        mesh_.AddTriangle(base + 1, base + 2, base + 3);
      }

      // If there is a previous character then adjust the character bounds 
      // to ensure they are both touching each other.  Do this by averaging 
      // the whitespace between the two characters.
      if(i != line.char_min_) {
        Character& p = characters_[i-1];
        noz_float p_max = p.bounds_.x + p.bounds_.w;
        noz_float c_min = c.bounds_.x;
        noz_float avg = (p_max + c_min) * 0.5f;
        p.bounds_.w = avg - p.bounds_.x;
        c.bounds_.w = (c.bounds_.x + c.bounds_.w) - avg;
        c.bounds_.x = avg;
      }
    }
  }
}

Vector2 TextNode::GetPositionFromCharIndex(noz_int32 index) {
  // If before the first character return the minimum position of the first line
  if(index<0) return lines_[0].bounds_.GetMin();

  // If past the last character return the end of the last line
  if(index>=(noz_int32)characters_.size()) {
    Line& line = lines_[lines_.size()-1];
    return Vector2(line.bounds_.x+line.bounds_.w,line.bounds_.y);
  }

  // Return the character min      
  return characters_[index].bounds_.GetMin();
}

noz_int32 TextNode::GetCharIndexFromPosition(const Vector2& pos) {
  // Find the matching line vertically
  noz_uint32 l=0;
  for(;l < lines_.size(); l++) {
    Line& line = lines_[l];
    if(pos.y >= line.bounds_.y && pos.y <= line.bounds_.y + line.bounds_.h) break;
  }

  // Return the last character which is EOL if past the last line.
  if(l>=lines_.size()) return characters_.size() - 1;

  Line& line = lines_[l];

  // If the chacter is before the line horizontally return the first character in the line
  if(pos.x <= line.bounds_.x) return line.char_min_;

  // If the position is within the last character or after it then return the last character
  if(pos.x >= characters_[line.char_max_].bounds_.x) return line.char_max_+1;

  // Find the character within the line.
  for(noz_uint32 c=line.char_min_; c<line.char_max_; c++) {
    noz_float min = characters_[c].bounds_.x;
    noz_float max = min + characters_[c].bounds_.w;
    if(pos.x >= min && pos.x <= max) return c;
  }

  return line.char_max_;
}

bool TextNode::DrawMesh (RenderContext* rc) {
  return rc->Draw(&mesh_);
}

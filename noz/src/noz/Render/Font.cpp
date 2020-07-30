///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;

Font::Font(void) {
  ascent_ = 0.0f;
  descent_ = 0.0f;
  height_ = 0.0f;
  distance_field_ = false;
}

Font::~Font(void) {
  delete image_;
}

void Font::MeasureText (const char* s, noz_int32 len, noz_float& cx, noz_float& cy, noz_float scale) {
  cy=height_;
  cx=0;

  for(;*s&&len!=0;s++,len--) {
    if(*s=='\r') {
      if(*(s+1)=='\n') continue;
      cy+=height_;
      continue;
    }
    if(*s=='\n') {
      cy+=height_;
      continue;
    }

    cx+=GetAdvanceX(*s,*(s+1));
  }

  cx *= scale;
  cy *= scale;
}

noz_float Font::GetAdvanceX (char c, char n) {
  if(c<(char)glyph_min_ || c>(char)glyph_max_) {
    return 0.0f;
  }
  Glyph* g = &glyphs_[c - glyph_min_];
  noz_float advance = g->ax;

  if(n!=0) {
    auto it = kerning_map_.find(MakeKerningCode(c,n));
    if(it != kerning_map_.end()) {
      advance += it->second->x;
    }
  }

  return advance;
}


void Font::UpdateKerningMap(void) {
  kerning_map_.clear();
  for(auto& kerning : kerning_) {
    kerning_map_[kerning.code] = &kerning;
  }
}


Font* Font::GetDefaultFont(void) {
  static const Guid DefaultFontGuid = Guid::Parse("{43D6EDE8-4F80-4E38-94E3-6941D1E9407E}");
  return AssetManager::LoadAsset<Font>(DefaultFontGuid);
}

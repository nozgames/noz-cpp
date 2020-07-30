///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "StyleSheet.h"
#include "Style.h"

using namespace noz;

StyleSheet::StyleSheet(void) {
}

StyleSheet::~StyleSheet(void) {
}

Style* StyleSheet::FindStyle (Type* type) const {
  for(noz_uint32 i=styles_.size();i>0;i--) {
    if(styles_[i-1]->GetControlType()==type) return styles_[i-1];
  }
  return nullptr;
}

void StyleSheet::AddStyle(Style* style) {
  styles_.push_back(style);
}




///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include "LayoutLengthProperty.h"

using namespace noz;

bool LayoutLengthProperty::IsEqual (Object* t1, Object* t2) const {
  if(t1==nullptr||t2==nullptr) return false;
  return Get(t1) == Get(t2);
}

const LayoutLength& LayoutLengthProperty::Get(Object* t) const { 
  noz_assert(t);
  return LayoutLength::Empty;
}

bool LayoutLengthProperty::Deserialize (Object* target, Deserializer& ds) {
  noz_assert(target);

  if(ds.PeekValueFloat()) {
    noz_float v = 0.0f;
    if(!ds.ReadValueFloat(v)) return false;
    Set(target,LayoutLength(v));
    return true;
  }

  String s;
  if(!ds.ReadValueString(s)) return false;
  s.Trim();
  
  LayoutLength v;
  if(s.Equals("Auto", StringComparison::OrdinalIgnoreCase) || s.Equals("NaN", StringComparison::OrdinalIgnoreCase)) {
    v = LayoutLength(LayoutUnitType::Auto, 0.0f);
  } else if (!s.IsEmpty() && s[s.GetLength()-1] == '%') {
    v = LayoutLength(LayoutUnitType::Percentage, Float::Parse(s.Substring(0,s.GetLength()-1)));
  } else {
    v = LayoutLength(LayoutUnitType::Fixed, Float::Parse(s));
  }    

  Set(target,v);

  return true;
}

bool LayoutLengthProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);

  LayoutLength v = Get(target);

  // Fixed
  if(v.unit_type_ == LayoutUnitType::Fixed) {
    s.WriteValueFloat(v.value_);
    return true;
  }

  // Auto
  if(v.unit_type_ == LayoutUnitType::Auto) {
    s.WriteValueString("Auto");
    return true;
  }

  // Percentage
  s.WriteValueString(String::Format("%f%%", v.value_));
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Deserializer.h>
#include <noz/Serialization/Serializer.h>
#include "RectProperty.h"

using namespace noz;

static const Name NameX ("X");
static const Name NameY ("Y");
static const Name NameW ("W");
static const Name NameH ("H");

const Rect& RectProperty::Get(Object* target) const {
  return Rect::Empty;
}

bool RectProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  Rect value;
  if(!s.ReadStartObject()) return false;

  // Read all members as long as an end object isnt next
  while(!s.PeekEndObject()) {
    Name member;
    if(!s.ReadMember(member)) return false;

    noz_float f;
    if(!s.ReadValueFloat(f)) return false;

    if(member==NameX) {value.x=f; continue;}
    if(member==NameY) {value.y=f; continue;}
    if(member==NameW) {value.w=f; continue;}
    if(member==NameH) {value.h=f; continue;}

    // TODO: erro
    return false;
  }

  if(!s.ReadEndObject()) return false;
  
  Set(target,value);
  return true;
}

bool RectProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);

  Rect r = Get(target);
  s.WriteStartObject();
  s.WriteMember(NameX);
  s.WriteValueFloat(r.x);
  s.WriteMember(NameY);
  s.WriteValueFloat(r.y);
  s.WriteMember(NameW);
  s.WriteValueFloat(r.w);
  s.WriteMember(NameH);
  s.WriteValueFloat(r.h);
  s.WriteEndObject ();

  return true;
}

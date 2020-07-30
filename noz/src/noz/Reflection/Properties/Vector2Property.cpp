///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include <noz/Animation/Vector2BlendTarget.h>
#include "Vector2Property.h"

using namespace noz;

static const Name NameX ("X");
static const Name NameY ("Y");

bool Vector2Property::IsEqual (Object* t1, Object* t2) const {
  if(t1==nullptr||t2==nullptr) return false;
  return Get(t1) == Get(t2);
}

const Vector2& Vector2Property::Get(Object* t) const { 
  noz_assert(t);
  return Vector2::Empty;
}

bool Vector2Property::Deserialize (Object* target, Deserializer& ds) {
  noz_assert(target);

  if(ds.PeekValueFloat()) {
    noz_float v = 0.0f;
    if(!ds.ReadValueFloat(v)) return false;
    Set(target,Vector2(v,v));
    return true;
  }

  Vector2 value;
  if(!ds.ReadStartObject()) return false;

  // Read all members as long as an end object isnt next
  while(!ds.PeekEndObject()) {
    Name member;
    if(!ds.ReadMember(member)) return false;

    noz_float f;
    if(!ds.ReadValueFloat(f)) return false;

    if(member==NameX) {value.x=f; continue;}
    if(member==NameY) {value.y=f; continue;}

    // TODO: error
    return false;
  }

  if(!ds.ReadEndObject()) return false;
  
  Set(target,value);
  return true;
}

bool Vector2Property::Serialize (Object* target, Serializer& s) {
  noz_assert(target);

  Vector2 v = Get(target);

  if(v.x == v.y) {
    s.WriteValueFloat(v.x);
    return true;
  }

  // Write as { "x": <v.x>, "y": <v.y> }
  s.WriteStartObject();
  s.WriteMember(NameX);
  s.WriteValueFloat(v.x);
  s.WriteMember(NameY);
  s.WriteValueFloat(v.y);
  s.WriteEndObject();

  return true;
}

const Name& Vector2Property::GetAnimationTrackName (noz_int32 i) const {
  noz_assert(i>=0&&i<=1);
  static const Name names[2] = { Name("X"),Name("Y")};
  return names[i];
}

BlendTarget* Vector2Property::CreateBlendTarget (Object* target) const {
  return new Vector2BlendTarget(target, (Property*)this);
}


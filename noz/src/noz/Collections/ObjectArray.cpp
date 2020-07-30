///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ObjectArray.h"

using namespace noz;

void ObjectArray::operator+= (Object* o) {
  objects_.push_back(o);
}

bool ObjectArray::Contains (Type* t) const {
  for(noz_uint32 i=0,c=objects_.size();i<c;i++) {
    Object* o = objects_[i];
    if(o && o->IsTypeOf(t)) return true;
  }
  return false;
}        


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryWriter.h>
#include <noz/IO/BinaryReader.h>

#include "NodePath.h"

using namespace noz;

NodePath::NodePath(void) {
  names_.resize(1);
  names_[0] = Name::Empty;
  relative_ = false;
}

NodePath::NodePath(const Name& name) {
  *this = name;
}

NodePath& NodePath::operator= (const Name& name) {
  names_.resize(1);
  names_[0] = name;
  return *this;
}

NodePath& NodePath::operator= (const char* path) {
  noz_int32 start = 0;

  // Counter the number of parts by counding the slashes.
  noz_int32 count = 1;
  const char* p;
  for(p = path+1;*p;p++) count += (*p=='/');  

  // Path is relative if it does not start with a slash.
  relative_ = path[0] != '/';

  // Set the array size to the exact size to conserve memory
  names_.resize(count);
  
  // Now extract the names..  
  count = 0;
  for(p = path + !relative_; *p; ) {
    const char* e;
    for(e=p; *e && *e != '/'; e++);
    names_[count++] = String(p, 0, e-p);
    p = e + (*e=='/');
  }

  return *this;
}

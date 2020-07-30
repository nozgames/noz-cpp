///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

#include "PropertyPath.h"

using namespace noz;

PropertyPath::PropertyPath(void) {
  names_.resize(1);
  names_[0] = Name::Empty;
}

PropertyPath::PropertyPath(const Name& name) {
  *this = name;
}

PropertyPath& PropertyPath::operator= (const Name& name) {
  names_.resize(1);
  names_[0] = name;
  return *this;
}

PropertyPath& PropertyPath::operator= (const char* path) {
  noz_int32 start = 0;

  // Counter the number of parts by counding the slashes.
  noz_int32 count = 1;
  const char* p;
  for(p = path;*p;p++) count += (*p=='.');  

  // Set the array size to the exact size to conserve memory
  names_.resize(count);
  
  // Now extract the names..  
  count = 0;
  for(p = path; *p; ) {
    const char* e;
    for(e=p; *e && *e != '.'; e++);
    names_[count++] = String(p, 0, e-p);
    p = e + (*e=='.');
  }

  return *this;
}


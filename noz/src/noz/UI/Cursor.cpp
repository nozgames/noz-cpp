///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Platform/CursorHandle.h>
#include "Cursor.h"

using namespace noz;


Cursor::Cursor(void) {
  handle_ = nullptr;
}

Cursor::~Cursor (void) {
  delete handle_;
}



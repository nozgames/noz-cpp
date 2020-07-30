///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Platform/CursorHandle.h>
#include "SystemCursor.h"

using namespace noz;
using namespace noz::Platform;

SystemCursor::SystemCursor(void) {
  cursor_type_ = SystemCursorType::Default;
  handle_ = CursorHandle::CreateInstance(this);  
}

SystemCursor::SystemCursor(SystemCursorType type) {
  cursor_type_ = type;
  handle_ = CursorHandle::CreateInstance(this);  
}

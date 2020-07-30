///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "GlueGen.h"

using namespace noz;
using namespace noz::Editor;

bool GlueType::IsCastableTo(GlueClass* gc) const {
  return gc_ && gc_->IsTypeOf(gc);
}

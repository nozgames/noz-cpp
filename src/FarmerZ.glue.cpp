///////////////////////////////////////////////////////////////////////////////
// Troglobite
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include "FarmerZ.pch.h"

using namespace noz;

#include <FarmerZ.glue.h>

extern void FarmerZ_RegisterTypes (void);
extern void noz_RegisterTypes (void);

namespace noz {
  void RegisterTypes (void) {
    noz_RegisterTypes ();
    FarmerZ_RegisterTypes();
  }
}

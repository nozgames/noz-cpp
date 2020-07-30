///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <Windows.h>
#include <noz/UI/Screen.h>

using namespace noz;

noz_float Screen::GetWidth(void) {
  return (noz_float)750.0f; // ::GetSystemMetrics(SM_CXSCREEN);
}

noz_float Screen::GetHeight(void) {
  return (noz_float)1334.0f; // ::GetSystemMetrics(SM_CYSCREEN);
}



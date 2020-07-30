///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Mutex.h"
#include <noz/Platform/MutexHandle.h>

using namespace noz;

Mutex::Mutex ( void ) {
  handle_ = Platform::MutexHandle::CreateInstance ();
}

Mutex::~Mutex ( void ) {
  delete handle_;
}

bool Mutex::WaitOne (void) {
  if (nullptr==handle_) return false;
  return handle_->WaitOne(-1);
}

bool Mutex::WaitOne (noz_int32 timeout_ms) {
  if (nullptr==handle_) return false;
  return handle_->WaitOne(timeout_ms);
}

void Mutex::Release (void) {
  if(nullptr==handle_) return;
  handle_->Release ();
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Semaphore.h"
#include <noz/Platform/SemaphoreHandle.h>

using namespace noz;


Semaphore::Semaphore (void) {
  handle_ = Platform::SemaphoreHandle::CreateInstance();
}
  
Semaphore::Semaphore (noz_int32 count) {
  handle_ = Platform::SemaphoreHandle::CreateInstance();
}

Semaphore::~Semaphore (void) {
  delete handle_;
}


bool Semaphore::WaitOne (void) {
  return false;
}

bool Semaphore::WaitOne (noz_int32 timeout_ms) {
  return false;
}

void Semaphore::Release (void) {  
}

void Semaphore::Release (noz_int32 count) {  
}

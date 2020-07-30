///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Process.h"
#include <noz/Platform/ProcessHandle.h>

using namespace noz;
using namespace noz::Platform;


Process::Process(void) {
  handle_ = nullptr;
}

Process::~Process(void) {
  delete handle_;
}

Process* Process::Start (const char* path, const char* args) {
  ProcessHandle* handle = ProcessHandle::CreateInstance(path, args);
  if(nullptr == handle) {
    return nullptr;
  }

  Process* process = new Process;
  process->handle_ = handle;
  return process;
}

bool Process::HasExited (void) const {
  return nullptr == handle_ || handle_->HasExited();
}

void Process::WaitForExit (void) {
  if(nullptr == handle_) return;
  handle_->WaitForExit();
}

bool Process::WaitForExit (noz_uint32 milliseconds) {
  if(nullptr == handle_) return true;
  return handle_->WaitForExit(milliseconds);
}


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Environment.h"

using namespace noz;

Environment* Environment::this_ = nullptr;


Environment::Environment(void) {
}

void Environment::Initialize (int argc, const char* argv[]) {
  if(this_ != nullptr) return;

  this_ = new Environment;

  // Copy command line arguments..
  this_->args_.resize(argc);
  for(int i=0;i<argc;i++) {
    this_->args_[i] = argv[i];
  }

  this_->executable_path_ = Path::Combine(Environment::GetCurrentDirectory(), argv[0]);
  this_->executable_dir_ = Path::GetDirectoryName(this_->executable_path_);
  this_->executable_name_ = Path::GetFilenameWithoutExtension(this_->executable_path_);
}

void Environment::Uninitialize(void) {
  delete this_;
  this_ = nullptr;
}

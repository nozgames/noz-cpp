///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonDeserializer.h>
#include "ApplicationSettings.h"

using namespace noz;

ApplicationSettings* ApplicationSettings::this_ = nullptr;

ApplicationSettings::ApplicationSettings(void) {
}

void ApplicationSettings::Initialize (void) {
  if(nullptr != this_) return;

  this_ = new ApplicationSettings;

  FileStream fs;
  if(!fs.Open(Path::Combine(Environment::GetFolderPath(SpecialFolder::Application),"Application.nozsettings"),FileMode::Open)) {
    return;
  }

  JsonDeserializer().Deserialize(&fs, this_);

  if(this_->name_.IsEmpty()) this_->name_ = Application::GetName();
}

void ApplicationSettings::Uninitialize (void) {
  if(nullptr == this_) return;
  delete this_;
  this_ = nullptr;
}



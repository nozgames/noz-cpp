///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonDeserializer.h>
#include "EditorSettings.h"

using namespace noz;

EditorSettings* EditorSettings::this_ = nullptr;

EditorSettings::EditorSettings(void) {
}

void EditorSettings::Initialize (void) {
  if(nullptr != this_) return;

  this_ = new EditorSettings;

  FileStream fs;
  if(!fs.Open(Path::Combine(Environment::GetFolderPath(SpecialFolder::Application),"Editor.nozsettings"),FileMode::Open)) {
    return;
  }

  JsonDeserializer().Deserialize(&fs, this_);

  // Cleanup the makefile path and add in the current directory
  if(!this_->makefile_path_.IsEmpty()) {
    this_->makefile_path_ = Path::Canonical(Path::Combine(Environment::GetCurrentDirectory(),this_->makefile_path_));
  }

  AddResolution ("iPhone 4 Portrait", 640, 960);
  AddResolution ("iPhone 5 Portrait", 640, 1136);
  AddResolution ("iPhone 6 Portrait", 750, 1334);
  AddResolution ("iPhone 6 Plus Portrait", 1242, 2208);
  AddResolution ("iPad Mini Portrait", 768, 1024);
}

void EditorSettings::Uninitialize (void) {
  if(nullptr == this_) return;
  delete this_;
  this_ = nullptr;
}



///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Prefs.h"

#include <noz/IO/Directory.h>
#include <noz/IO/BinaryWriter.h>
#include <noz/IO/BinaryReader.h>

using namespace noz;


Prefs* Prefs::this_ = nullptr;

void Prefs::Initialize(void) {
  if(this_!=nullptr) return;

  this_ = new Prefs;

  this_->path_ = Path::Combine(Environment::GetFolderPath(SpecialFolder::ApplicationSupport),"User.nozprefs");

  FileStream fs;
  if(fs.Open(this_->path_, FileMode::Open)) {
    BinaryReader reader(&fs);
    while(1) {
      Name name = reader.ReadName();
      if(name.IsEmpty()) break;
      this_->prefs_[name] = reader.ReadString();
    }
  }
}

void Prefs::Uninitialize(void) {
  if(this_==nullptr) return;

  // Save all unsaved preferences
  Save();

  delete this_;
}

noz_float Prefs::GetFloat(const Name& name, noz_float def) {
  auto it = this_->prefs_.find(name);
  if(it != this_->prefs_.end()) {
    return Float::Parse(it->second);
  }
  return def;
}
 
noz_int32 Prefs::GetInt32 (const Name& name, noz_int32 def) {
  auto it = this_->prefs_.find(name);
  if(it != this_->prefs_.end()) {
    return Int32::Parse(it->second);
  }
  return def;
}

void Prefs::SetFloat(const Name& name, noz_float value) {
  this_->prefs_[name] = Float(value).ToString();
}

void Prefs::SetInt32(const Name& name, noz_int32 value) {
  this_->prefs_[name] = Int32(value).ToString();
} 

void Prefs::Save (void) {
  FileStream fs;
  if(!fs.Open(this_->path_, FileMode::Truncate)) {
    return;
  }

  BinaryWriter writer(&fs);
  for(auto it=this_->prefs_.begin(); it!=this_->prefs_.end(); it++) {
    writer.WriteName(it->first);
    writer.WriteString(it->second);
  }
  writer.WriteName(Name::Empty);  
}

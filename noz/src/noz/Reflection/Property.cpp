///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;
  
Property::Property(PropertyAttributes attr) {
  type_ = nullptr;
  attributes_ = attr;
}

bool Property::IsDefault (Object* target) const {
  if(target==nullptr) return false;

  // Get the defaults object from the type of the target. 
  Object* defaults = target->GetType()->GetDefaults();

  // If there is no defaults object then we have to assume that all 
  // properties on the type are non-default
  if(defaults == nullptr) return false;

  // Call into the derrived version of the method to get the official answer.
  return IsEqual(target,defaults);
}

void Property::SetMeta (const Name& name, const char* value) {
  meta_[name] = value;
}

const char* Property::GetMeta (const Name& name, const char* def) const {
  auto it = meta_.find(name);
  if(it != meta_.end()) {
    return it->second.ToCString();
  }
  return def;
}


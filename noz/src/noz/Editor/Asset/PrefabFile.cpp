///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/Prefab.h>
#include <noz/Serialization/JsonDeserializer.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Serialization/BinarySerializer.h>
#include "PrefabFile.h"

using namespace noz;
using namespace noz::Editor;

Asset* PrefabFile::Import (void) {
  // Allocate the prefab template
  Prefab* prefab = new Prefab;

  if(!Import(prefab)) {
    delete prefab;
    return nullptr;
  }
 
  // Return the asset.
  return (Asset*)prefab;
}

Node* PrefabFile::CreateNode (void) const {
  Prefab* t = AssetManager::LoadAsset<Prefab>(GetGuid());
  if(nullptr == t) {
    return nullptr;
  }

  return t->Instantiate();
}

bool PrefabFile::CreateEmpty (void) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Truncate)) return nullptr;

  Prefab::Def def;
  JsonSerializer().Serialize(&def, &fs);
  fs.Close();
  
  return true;
}

bool PrefabFile::Reimport (Asset* asset) {
  Prefab* prefab = Cast<Prefab>(asset);
  if(nullptr == prefab) return false;

  prefab->instance_.Set(nullptr);
  prefab->properties_.clear();

  if(!Import(prefab)) return false;
  
  // Iterate over all known objects and set new asset
  for(noz_uint32 i=1,c=ObjectManager::GetObjectCount(); i<c; i++) {
    Object* o = ObjectManager::GetObject(i);
    if(o==nullptr) continue;

    if(!o->IsTypeOf(typeof(PrefabNode))) continue;

    ((PrefabNode*)o)->Reload();    
  }

  return true;
}

bool PrefabFile::Import (Prefab* prefab) {
  // Load the prefab stream
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Open)) {
    return nullptr;
  }

  // Deserialize the stream into a prefab template definition object
  Prefab::Def def;
  if(nullptr==JsonDeserializer().Deserialize(&fs,&def)) {
    return nullptr;
  }

  // Create a template instance 
  Prefab::Instance instance;

  // Map of object identifier to object index.  Using a map ensures that if multiple
  // properties from the same object are exported that the object isnt represented in
  // the object table more than once.
  std::map<noz_uint32,noz_uint32> object_map_;

  // populate the properties
  prefab->properties_.reserve(def.properties_.size());
  for(auto itp=def.properties_.begin(); itp!=def.properties_.end(); itp++) {
    const PropertyProxy::Def& pdef = *itp;

    // Check for missing name
    if(pdef.name_.IsEmpty()) {
      Console::WriteLine("%s: error: missing name on export", GetPath().ToCString());
      continue;
    }

    // Check for invalid object.
    if(pdef.exported_object_==nullptr) {
      Console::WriteLine("%s: error: invalid object for export '%s'", GetPath().ToCString(), pdef.name_.ToCString());
      continue;
    }

    // Extract the property using the property name
    Property* prop = pdef.exported_object_->GetType()->GetProperty(pdef.exported_property_);
    if(nullptr==prop) {
      Console::WriteLine("%s: error: unknown property '%s' on exported object", GetPath().ToCString(), pdef.exported_property_.ToCString());
      continue;
    }

    // Assign the object an object identifer within the template
    noz_uint32 object_id;
    auto ito=object_map_.find(pdef.exported_object_->GetId());
    if(ito==object_map_.end()) {
      object_id = instance.objects_.size();
      object_map_[pdef.exported_object_->GetId()] = object_id;
      instance.objects_.push_back(pdef.exported_object_);
    } else {
      object_id = ito->second;
    }

    // Add the property to the template
    prefab->properties_.emplace_back(pdef.name_, object_id, prop);
  }

  instance.node_ = def.node_;

  prefab->instance_.Set(&instance);
  
  return true;  
}

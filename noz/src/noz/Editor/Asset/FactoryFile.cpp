///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonDeserializer.h>
#include "FactoryFile.h"

using namespace noz;
using namespace noz::Editor;


Asset* FactoryFile::Import (void) {
  // Open the control template definition file.
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Open)) return nullptr;

  // Deserialize the stream into a Factor::Def
  Factory::Def def;
  def.factory_type_ = nullptr;
  if(nullptr==JsonDeserializer().Deserialize(&fs,&def)) {
    return nullptr;
  }

  // Close the input file
  fs.Close();

  // Ensure a factory type was specified
  if(def.factory_type_ == nullptr) {
    Console::WriteLine("%s: error: missing or invalid FactoryType", GetPath().ToCString());
    return nullptr;
  }

  // Create the factory object
  Factory* factory = def.factory_type_->CreateInstance<Factory>();
  if(nullptr == factory) {
    Console::WriteLine("%s: error: failed to create instance of FactoryType '%s'", GetPath().ToCString(),def.factory_type_->GetQualifiedName().ToCString());
    return nullptr;
  }

  // Convert all objects in the def into serialized objects
  for(noz_uint32 i=0,c=def.objects_.size(); i<c; i++) {
    Factory::ObjectDef* odef = def.objects_[i];
    if(nullptr==odef || nullptr==odef->object_) continue;

    ObjectProperty* p = factory->GetType()->GetProperty<ObjectProperty>(odef->property_);
    if(p==nullptr) {
      Console::WriteLine("%s: warning: FactoryType '%s' does not contain a property named '%s'", 
        GetPath().ToCString(),def.factory_type_->GetQualifiedName().ToCString(),odef->property_.ToCString());
      continue;
    }

    // Ensure the object type is SerializedObject
    if(!p->GetObjectType()->IsCastableTo(typeof(SerializedObject))) {
      Console::WriteLine("%s: warning: property '%s' of FactoryType '%s' must be of type 'ObjectPtr<SerializedObject>'", 
        GetPath().ToCString(),odef->property_.ToCString(),def.factory_type_->GetQualifiedName().ToCString());
      continue;
    }

    // Create the serialized object
    ((SerializedObject*)p->Get(factory))->Set(odef->object_);
  }

  // Return the factory
  return factory;
}

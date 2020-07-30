///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/UI/ControlTemplate.h>
#include <noz/Serialization/JsonDeserializer.h>
#include <noz/Serialization/BinarySerializer.h>
#include "ControlTemplateImporter.h"

using namespace noz;


Asset* ControlTemplateImporter::Import (const String& path) {
  // Open the control template definition file.
  FileStream fs;
  if(!fs.Open(path,FileMode::Open)) return nullptr;

  // Deserialize the stream into a ControlTemplateDef object
  ControlTemplate::Def def;
  if(nullptr==JsonDeserializer().Deserialize(&fs,&def)) {
    return nullptr;
  }

  // Close the input file
  fs.Close();

  // Create the control template
  ControlTemplate* ct = new ControlTemplate;

  // Create a template instance 
  ControlTemplate::Instance instance;

  // Map of object identifier to object index.  Using a map ensures that if multiple
  // properties from the same object are exported that the object isnt represented in
  // the object table more than once.
  std::map<noz_uint32,noz_uint32> object_map_;

  // populate the properties
  ct->properties_.reserve(def.properties_.size());
  for(auto itp=def.properties_.begin(); itp!=def.properties_.end(); itp++) {
    const PropertyProxy::Def& pdef = *itp;

    // Check for missing name
    if(pdef.name_.IsEmpty()) {
      Console::WriteLine("%s: error: missing name on export", path.ToCString());
      continue;
    }

    // Check for invalid object.
    if(pdef.exported_object_==nullptr) {
      Console::WriteLine("%s: error: invalid object for export '%s'", path.ToCString(), pdef.name_.ToCString());
      continue;
    }

    // Extract the property using the property name
    Property* prop = pdef.exported_object_->GetType()->GetProperty(pdef.exported_property_);
    if(nullptr==prop) {
      Console::WriteLine("%s: error: unknown property '%s' on exported object", path.ToCString(), pdef.exported_property_.ToCString());
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
    ct->properties_.emplace_back(pdef.name_, object_id, prop);
  }

  // Copy the parts.
  instance.parts_ = def.parts_;

  // Copy the nodes to the instance
  instance.nodes_ = def.nodes_;

  // Copy the nodes to the instance
  ct->factory_ = def.factory_;

  // Copy the control type..
  NOZ_TODO("handle nullptr on control type");
  ct->control_type_ = def.control_type_;

  // Serialize the instance to the control template
  MemoryStream ms(4096);
  BinarySerializer().Serialize(&instance,&ms);  
  ct->instance_.resize(ms.GetLength());
  memcpy(&ct->instance_[0], ms.GetBuffer(), ms.GetLength());

  // Return the template
  return ct;
}

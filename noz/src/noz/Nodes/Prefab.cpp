///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryReader.h>
#include <noz/Serialization/BinarySerializer.h>
#include <noz/Serialization/BinaryDeserializer.h>

#include "Prefab.h"
#include "PrefabNode.h"

using namespace noz;

Prefab::Prefab(void) {
}

Prefab::~Prefab(void) {
  NOZ_TODO("Free type?");
}

PrefabNode* Prefab::Instantiate (const Name& name, NodeAttributes attr) {
  PrefabNode* node = prefab_node_type_->CreateInstance<PrefabNode>();
  if(nullptr == node) {
    return nullptr;
  }

  NOZ_TODO("attr?");

  node->SetName(name);

  return node;
}

void Prefab::OnLoaded (void) {
  class Allocator : public ObjectAllocator {
    public: Guid prefab_guid_;
    public: Allocator(const Guid& guid) : prefab_guid_(guid) {}
    public: virtual Object* CreateInstance(ObjectAllocatorAttributes attr) override {
      Prefab* prefab = AssetManager::LoadAsset<Prefab>(prefab_guid_);
      if(nullptr == prefab) {
        return nullptr;
      }
      PrefabNode* result = new PrefabNode;
      result->ApplyTemplate(prefab);
      return result;
    }
  };

  // Generate the dynamic type name for the prefab.
  Name type_name = String::Format("{noz::Prefab@%s}", GetGuid().ToString().ToCString());

  // Create the prefab type
  prefab_node_type_ = new Type(type_name.ToCString(), 0, TypeCode::Class, typeof(PrefabNode));
  prefab_node_type_->SetAllocator(new Allocator(GetGuid()));

  // Register prefab type properties
  for(auto it=properties_.begin(); it!=properties_.end(); it++) {
    PropertyProxy& pp = *it;

    // Ensure there is a valid property to proxy
    if(pp.GetProxyProperty()==nullptr) continue;

    // Create the proxy
    Property* p = pp.GetProxyProperty()->CreateProxy(&pp);
    if(nullptr==p) {
      Console::WriteLine("%s: warning: property '%s' could not be exported (property type '%s' does not support proxies)",
        GetName().ToCString(), pp.GetProxyProperty()->GetName().ToCString(), pp.GetProxyProperty()->GetType()->GetQualifiedName().ToCString()
      );
      continue;
    }

    // Register the property..
    prefab_node_type_->RegisterProperty(pp.GetName(),p);
  }

  // Register the prefab type
  Type::RegisterType(prefab_node_type_);
}

bool Prefab::Instance::DeserializeNode (Deserializer& s) {
  Object* o = nullptr;
  if(!s.ReadValueObject(o,typeof(Node))) return false;    

  if(o!=nullptr) {
    noz_assert(target_);
    target_->instance_ = (Node*)o;
  }

  return true;
}

bool Prefab::Instance::DeserializeObjects (Deserializer& s) {
  NOZ_FIXME()
  // Read array size.
  noz_uint32 size;
  if(!s.ReadStartSizedArray(size)) return false;  

  target_->template_objects_.reserve(size);

  // Deserialize list of proxy objects directly into target list.
  for(noz_uint32 i=0; i<size; i++) {
    Object* o = nullptr;
    if(!s.ReadValueObject(o,typeof(Object))) return false;    
    target_->template_objects_.push_back(o);
  }

  // read end array
  if(!s.ReadEndArray()) return false;  

  return true;
}

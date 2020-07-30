///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "ObjectPtrVectorProperty.h"

using namespace noz;

bool ObjectPtrVectorProperty::Deserialize (Object* t, Deserializer& s) {
  noz_uint32 size;
  if(!s.ReadStartSizedArray(size)) return false;  

  Reserve(t,size);

  for(noz_uint32 i=0; i<size; i++) {
    Object* o = nullptr;
    if(!s.ReadValueObject(o,GetObjectType())) return false;
    if(o) Add(t,o);
  }

  if(!s.ReadEndArray()) return false;

  return true;
}

bool ObjectPtrVectorProperty::Serialize (Object* t, Serializer& s) {
  noz_uint32 count = GetCount(t);
  s.WriteStartSizedArray(count);
  for(noz_uint32 i=0;i<count;i++) {
    s.WriteValueObject(Get(t,i));
  }
  s.WriteEndArray();
  return true;
}

Property* ObjectPtrVectorProperty::CreateProxy (PropertyProxy* proxy) const {
  class ObjectPtrVectorPropertyProxy : public ObjectPtrVectorProperty {
    protected: ObjectPtr<PropertyProxy> proxy_;
    public: ObjectPtrVectorPropertyProxy(PropertyProxy* pp) : proxy_(pp) {}
    public: virtual noz_uint32 GetCount (Object* t) const override {
      return proxy_->GetProxyProperty<ObjectPtrVectorProperty>()->GetCount(proxy_->GetProxyObject(t));
    }
    public: virtual void Reserve (Object* t, noz_uint32 capacity) override {
      proxy_->GetProxyProperty<ObjectPtrVectorProperty>()->Reserve(proxy_->GetProxyObject(t),capacity);
    }
    public: virtual Object* Get (Object* t, noz_uint32 index) const override {
      return proxy_->GetProxyProperty<ObjectPtrVectorProperty>()->Get(proxy_->GetProxyObject(t),index);
    }
    public: virtual void Add (Object* t, Object* value) override {
      proxy_->GetProxyProperty<ObjectPtrVectorProperty>()->Add(proxy_->GetProxyObject(t),value);
    }
    public: virtual void Remove (Object* t, noz_uint32 index) override {
      proxy_->GetProxyProperty<ObjectPtrVectorProperty>()->Get(proxy_->GetProxyObject(t),index);
    }
    public: virtual void Insert (Object* t, noz_uint32 index, Object* value) override {
      proxy_->GetProxyProperty<ObjectPtrVectorProperty>()->Insert(proxy_->GetProxyObject(t),index,value);
    }
    public: virtual void Clear (Object* t) override {
      proxy_->GetProxyProperty<ObjectPtrVectorProperty>()->Clear(proxy_->GetProxyObject(t));
    }
    public: virtual Object* Release (Object* t, noz_uint32 index) override {
      return proxy_->GetProxyProperty<ObjectPtrVectorProperty>()->Get(proxy_->GetProxyObject(t),index);
    }
    public: virtual Type* GetObjectType (void) const override {
      return proxy_->GetProxyProperty<ObjectPtrVectorProperty>()->GetObjectType();
    }
  };
  return new ObjectPtrVectorPropertyProxy(proxy);
}

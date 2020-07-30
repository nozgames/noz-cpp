///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Deserializer.h>
#include <noz/Serialization/Serializer.h>
#include "StringProperty.h"

using namespace noz;

bool StringProperty::IsEqual (Object* t1, Object* t2) const {
  if(t1==nullptr||t2==nullptr) return false;
  return Get(t1).Equals(Get(t2));
}

String StringProperty::Get (Object* target) const {
  return String();
}

bool StringProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  String value;
  if(!s.ReadValueString(value)) return false;
  Set (target,value);
  return true;
}

bool StringProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueString(Get(target));
  return true;
}

Property* StringProperty::CreateProxy (PropertyProxy* proxy) const {
  class StringPropertyProxy : public StringProperty {
    protected: ObjectPtr<PropertyProxy> proxy_;
    public: StringPropertyProxy(PropertyProxy* proxy) : proxy_(proxy) {}
    public: virtual void Set(Object* t, const String& value) override {
      proxy_->GetProxyProperty<StringProperty>()->Set(proxy_->GetProxyObject(t),value);
    }
    public: virtual String Get(Object* t) const override {
      return proxy_->GetProxyProperty<StringProperty>()->Get(proxy_->GetProxyObject(t));
    }
  };
  return new StringPropertyProxy(proxy);
}

bool StringVectorProperty::Deserialize (Object* target, Deserializer& s) {
  std::vector<String>& v = Get (target);

  noz_uint32 size = 0;
  if(!s.ReadStartSizedArray(size)) return false;

  v.clear();
  v.reserve(size);

  while(!s.PeekEndArray()) {
    String value;
    if(!s.ReadValueString(value)) return false;
    v.push_back(value);
  }

  return s.ReadEndArray(); 
}

bool StringVectorProperty::Serialize (Object* target, Serializer& s) {
  std::vector<String>& v = Get (target);
  s.WriteStartSizedArray(v.size());
  for(noz_uint32 i=0,c=v.size(); i<c; i++) {
    s.WriteValueString(v[i]);
  }
  s.WriteEndArray();
  return true;
}

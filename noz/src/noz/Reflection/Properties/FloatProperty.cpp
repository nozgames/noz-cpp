///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Animation/FloatBlendTarget.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "FloatProperty.h"

using namespace noz;

BlendTarget* FloatProperty::CreateBlendTarget (Object* target) const {
  return new FloatBlendTarget(target,(Property*)this);
}

bool FloatProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);
  noz_float v;
  if(!s.ReadValueFloat(v)) return false;
  Set(target,v);
  return true;
}

bool FloatProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueFloat(Get(target));
  return true;
}

Property* FloatProperty::CreateProxy (PropertyProxy* proxy) const {
  class FloatPropertyProxy : public FloatProperty {
    protected: ObjectPtr<PropertyProxy> proxy_;
    public: FloatPropertyProxy(PropertyProxy* pp) : proxy_(pp) {}
    public: virtual void Set(Object* t, noz_float value) override {
      proxy_->GetProxyProperty<FloatProperty>()->Set(proxy_->GetProxyObject(t),value);
    }
    public: virtual noz_float Get(Object* t) const override {
      return proxy_->GetProxyProperty<FloatProperty>()->Get(proxy_->GetProxyObject(t));
    }
  };
  return new FloatPropertyProxy(proxy);
}


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Animation/SpriteBlendTarget.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include "ObjectPtrProperty.h"

using namespace noz;

BlendTarget* ObjectPtrProperty::CreateBlendTarget (Object* target) const {
  if(GetObjectType()->IsCastableTo(typeof(Sprite))) {
    return new SpriteBlendTarget(target, (Property*)this);
  }

  return nullptr;
}

bool ObjectPtrProperty::IsEqual (Object* t1, Object* t2) const {
  if(t1==nullptr||t2==nullptr) return false;
  return Get(t1) == Get(t2);
}

bool ObjectPtrProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);

  Object* o = nullptr;
  if(!s.ReadValueObject(o,GetObjectType())) return false;
  Set(target,o);
  return true;
}

bool ObjectPtrProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueObject(Get(target));
  return true;
}

Property* ObjectPtrProperty::CreateProxy (PropertyProxy* proxy) const {
  class ObjectPtrPropertyProxy : public ObjectPtrProperty {
    protected: ObjectPtr<PropertyProxy> proxy_;
    public: ObjectPtrPropertyProxy(PropertyProxy* pp) : proxy_(pp) {}
    public: virtual Type* GetObjectType(void) const {
      return proxy_->GetProxyProperty<ObjectPtrProperty>()->GetObjectType();
    }
    public: virtual void Set(Object* t, Object* v) override {
      return proxy_->GetProxyProperty<ObjectPtrProperty>()->Set(proxy_->GetProxyObject(t),v);
    }
    public: virtual Object* Get(Object* t) const override {
      return proxy_->GetProxyProperty<ObjectPtrProperty>()->Get(proxy_->GetProxyObject(t));
    }
  };
  return new ObjectPtrPropertyProxy(proxy);
}

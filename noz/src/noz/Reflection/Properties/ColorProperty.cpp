///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>
#include <noz/Animation/ColorBlendTarget.h>
#include "ColorProperty.h"

using namespace noz;


Color ColorProperty::Get (Object* target) const {
  return Color::White;
}

bool ColorProperty::IsEqual (Object* t1, Object* t2) const {
  if(t1==nullptr||t2==nullptr) return false;
  return Get(t1) == Get(t2);
}

bool ColorProperty::Deserialize (Object* target, Deserializer& s) {
  noz_assert(target);
  Color v;
  if(!s.ReadValueColor(v)) return false;
  Set(target,v);
  return true;
}

bool ColorProperty::Serialize (Object* target, Serializer& s) {
  noz_assert(target);
  s.WriteValueColor(Get (target));
  return true;
}

Property* ColorProperty::CreateProxy (PropertyProxy* proxy) const {
  class ColorPropertyProxy : public ColorProperty {
    protected: ObjectPtr<PropertyProxy> proxy_;
    public: ColorPropertyProxy(PropertyProxy* proxy) : proxy_(proxy) {}
    public: virtual void Set(Object* t, Color value) override {
      proxy_->GetProxyProperty<ColorProperty>()->Set(proxy_->GetProxyObject(t),value);
    }
    public: virtual Color Get(Object* t) const override {
      return proxy_->GetProxyProperty<ColorProperty>()->Get(proxy_->GetProxyObject(t));
    }
  };
  return new ColorPropertyProxy(proxy);
}

const Name& ColorProperty::GetAnimationTrackName (noz_int32 i) const {
  noz_assert(i >= 0 && i <= 3);
  static const Name names[4]={Name("R"),Name("G"),Name("B"), Name("A")};
  return names[i];
}

BlendTarget* ColorProperty::CreateBlendTarget (Object* target) const {
  return new ColorBlendTarget(target, (Property*)this);
}


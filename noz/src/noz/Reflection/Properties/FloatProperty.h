///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_FloatProperty_h__
#define __noz_FloatProperty_h__

namespace noz {

  class FloatProperty : public Property {
    NOZ_OBJECT(TypeCode=FloatProperty)

    public: FloatProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Return the property value from the given target
    public: virtual noz_float Get (Object* t) const {return 0;}

    /// Set the property value on the given target
    public: virtual void Set (Object* t, noz_float v) = 0;

    /// Return true if the property value is the same within both targets
    public: virtual bool IsEqual (Object* t1, Object* t2) const override {
      if(t1==nullptr||t2==nullptr) return false;
      return Get(t1) == Get(t2);
    }

    public: virtual Property* CreateProxy (PropertyProxy* proxy) const override;

    public: virtual BlendTarget* CreateBlendTarget (Object* target) const override;

    public: virtual noz_int32 GetAnimationTrackCount (void) const {return 1;}

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };
  
}

#endif // __noz_FloatProperty_h__

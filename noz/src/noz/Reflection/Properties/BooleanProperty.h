///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_BooleanProperty_h__
#define __noz_BooleanProperty_h__

namespace noz {

  class BooleanProperty : public Property {
    NOZ_OBJECT(TypeCode=BooleanProperty,NoAllocator)

    public: BooleanProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }
    
    public: virtual bool Get (Object* owner) const = 0;

    public: virtual void Set (Object* owner, bool value) = 0;

    /// Return true if the property value is the same within both targets
    public: virtual bool IsEqual (Object* t1, Object* t2) const override {
      if(t1==nullptr||t2==nullptr) return false;
      return Get(t1) == Get(t2);
    }

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;

    public: virtual BlendTarget* CreateBlendTarget (Object* target) const override;
  };

}

#endif // __noz_BooleanProperty_h__

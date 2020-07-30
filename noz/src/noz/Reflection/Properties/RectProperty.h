///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RectProperty_h__
#define __noz_RectProperty_h__

namespace noz {

  class RectProperty : public Property {
    NOZ_OBJECT(NoAllocator,TypeCode=RectProperty)

    public: RectProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Returns the value of the float property..
    public: virtual const Rect& Get (Object* target) const;

    public: virtual void Set (Object* target, const Rect& value) = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override {
      if(t1==nullptr||t2==nullptr) return false;
      return Get(t1) == Get(t2);
    }

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };

}

#endif // __noz_RectProperty_h__

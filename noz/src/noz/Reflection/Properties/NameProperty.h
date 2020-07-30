///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_NameProperty_h__
#define __noz_NameProperty_h__

namespace noz {

  class NameProperty : public Property {
    NOZ_OBJECT(TypeCode=NameProperty,NoAllocator)

    public: NameProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual const Name& Get (Object* owner) const = 0;

    public: virtual void Set (Object* owner, const Name& value) = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override {
      if(t1==nullptr||t2==nullptr) return false;
      return Get(t1) == Get(t2);
    }

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };
}

#endif // __noz_NameProperty_h__

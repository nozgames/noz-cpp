///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_MethodProperty_h__
#define __noz_MethodProperty_h__

namespace noz {

  class MethodProperty : public Property {
    NOZ_OBJECT(TypeCode=MethodProperty)

    public: MethodProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual Method* Get (Object* target) const {return nullptr;}

    public: virtual void Set (Object* target, Method* v) = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };
}

#endif // __noz_MethodProperty_h__

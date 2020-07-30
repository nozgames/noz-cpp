///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_PropertyProperty_h__
#define __noz_PropertyProperty_h__

namespace noz {

  class PropertyProperty : public Property {
    NOZ_OBJECT(TypeCode=PropertyProperty)

    public: PropertyProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual Property* Get (Object* target) const {return nullptr;}

    public: virtual void Set (Object* target, Property* v) = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };
}

#endif // __noz_PropertyProperty_h__

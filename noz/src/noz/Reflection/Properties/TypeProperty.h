///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_TypeProperty_h__
#define __noz_TypeProperty_h__

namespace noz {

  class TypeProperty : public Property {
    NOZ_OBJECT(TypeCode=TypeProperty,NoAllocator)

    public: TypeProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Returns the property value.
    public: virtual Type* Get(Object* t) const = 0;

    /// Sets the property value.
    public: virtual void Set (Object* t, Type* v) = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };
}


#endif // __noz_TypeProperty_h__

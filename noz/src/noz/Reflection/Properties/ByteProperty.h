///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ByteProperty_h__
#define __noz_ByteProperty_h__

#include "VectorProperty.h"

namespace noz {

  class ByteProperty : public Property {
    NOZ_OBJECT(NoAllocator,TypeCode=ByteProperty)

    public: ByteProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual noz_byte Get (Object* target) const {return 0;}

    public: virtual void Set (Object* target, noz_byte value) = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };

  class ByteVectorProperty : public IntrinsicVectorProperty<noz_byte> {
    NOZ_OBJECT(Abstract,TypeCode=ByteVectorProperty)
    public: ByteVectorProperty(PropertyAttributes attr=PropertyAttributes::Default) : IntrinsicVectorProperty(attr) { }

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  }; 
}

#endif // __noz_ByteProperty_h__

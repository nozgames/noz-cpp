///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_UInt32Property_h__
#define __noz_UInt32Property_h__

namespace noz {

  class UInt32Property : public Property {
    NOZ_OBJECT(NoAllocator)

    public: UInt32Property(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual noz_uint32 Get (Object* target) const {return 0;}

    public: virtual void Set  (Object* target, noz_uint32 value) = 0;

    public: virtual bool Serialize (Object* target, Serializer& serializer) override;

    public: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };


} // namespace noz

#endif // __noz_UInt32Property_h__

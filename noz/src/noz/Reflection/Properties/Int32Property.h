///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Int32Property_h__
#define __noz_Int32Property_h__

namespace noz {


  class Int32Property : public Property {
    NOZ_OBJECT(TypeCode=Int32Property)

    public: Int32Property(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Retrieve the property value.
    public: virtual noz_int32 Get (Object* target) const = 0;

    /// Set the property value.
    public: virtual void Set (Object* target, noz_int32 value) = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override {
      if(t1==nullptr||t2==nullptr) return false;
      return Get(t1) == Get(t2);
    }

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };


} // namespace noz

#endif // __noz_Int32Property_h__

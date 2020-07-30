///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_GuidProperty_h__
#define __noz_GuidProperty_h__

namespace noz {


  class GuidProperty : public Property {
    NOZ_OBJECT(TypeCode=GuidProperty)

    public: GuidProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Retrieve the property value.
    public: virtual const noz::Guid& Get (Object* target) const = 0;

    /// Set the property value.
    public: virtual void Set (Object* target, const Guid& value) = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override {
      if(t1==nullptr||t2==nullptr) return false;
      return Get(t1) == Get(t2);
    }

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };


} // namespace noz

#endif // __noz_GuidProperty_h__

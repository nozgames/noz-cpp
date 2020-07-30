///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_PropertyPathProperty_h__
#define __noz_PropertyPathProperty_h__

namespace noz {

  class PropertyPathProperty : public Property {
    NOZ_OBJECT()

    public: PropertyPathProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual void Set (Object* t, const PropertyPath& v) = 0;

    public: virtual const PropertyPath& Get (Object* t) const;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };

} // namespace noz

#endif // __noz_PropertyPathProperty_h__

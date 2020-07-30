///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_StringProperty_h__
#define __noz_StringProperty_h__

namespace noz {

  class Vector2;
  class String;
  class Property;

  class StringProperty : public Property {
    NOZ_OBJECT(TypeCode=StringProperty)

    public: StringProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual String Get(Object* target) const;

    /// Set the value of the property from an object
    public: virtual void Set (Object* target, const String& value) = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override;

    public: virtual Property* CreateProxy (PropertyProxy* proxy) const override;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };

  class StringVectorProperty : public IntrinsicVectorProperty<String> {
    NOZ_OBJECT(Abstract)
    public: StringVectorProperty(PropertyAttributes attr=PropertyAttributes::Default) : IntrinsicVectorProperty(attr) { }

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  }; 
}

#endif // __noz_StringProperty_h__

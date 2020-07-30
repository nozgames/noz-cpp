///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_LayoutLengthProperty_h__
#define __noz_LayoutLengthProperty_h__

namespace noz {

  class LayoutLength;

  class LayoutLengthProperty : public Property {
    NOZ_OBJECT(NoAllocator,TypeCode=LayoutLengthProperty)

    /// Default constructor
    public: LayoutLengthProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Override to get the property value
    public: virtual const LayoutLength& Get (Object* target) const;

    /// Override to set the property value 
    public: virtual void Set (Object* target, const LayoutLength& value) = 0;

    /// Return true if the property value is the same within both targets
    public: virtual bool IsEqual (Object* t1, Object* t2) const override;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };

}

#endif // __noz_LayoutLengthProperty_h__

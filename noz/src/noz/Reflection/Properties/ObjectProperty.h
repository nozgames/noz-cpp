///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ObjectProperty_h__
#define __noz_ObjectProperty_h__

namespace noz {

  class ObjectProperty : public Property {
    NOZ_OBJECT(TypeCode=ObjectProperty)

    /// Default constructor
    public: ObjectProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Override to return the type of the object that the property references
    public: virtual Type* GetObjectType (void) const = 0;

    /// Return a class pointer for the member this property represents within the target
    public: virtual Object* Get (Object* target) const {return nullptr;}

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };
  

} // namespace noz

#endif // __noz_Property_h__

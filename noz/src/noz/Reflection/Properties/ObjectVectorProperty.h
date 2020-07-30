///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ObjectVectorProperty_h__
#define __noz_ObjectVectorProperty_h__

namespace noz {

  NOZ_TODO("Convert to same format as ObjectPtrVectorProperty");
  class ObjectVectorProperty : public Property {
    NOZ_OBJECT(TypeCode=ObjectVectorProperty)

    public: ObjectVectorProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual Object* AddElement (Object* target) {noz_assert(false);return nullptr;}

    public: virtual Object* GetElement(Object* target, noz_uint32 index) const {noz_assert(false);return nullptr;}

    public: virtual Type* GetElementType(void) const = 0;

    public: virtual noz_uint32 GetSize(Object* target) const {noz_assert(false); return 0;}

    public: virtual void SetSize(Object* target, noz_uint32 size) = 0;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };   

} // namespace noz

#endif // __noz_ObjectVectorProperty_h__

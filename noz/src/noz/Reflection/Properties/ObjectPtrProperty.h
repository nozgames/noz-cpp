///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ObjectPtrProperty_h__
#define __noz_ObjectPtrProperty_h__

namespace noz {

  class Component;

  class ObjectPtrProperty : public Property {
    NOZ_OBJECT(TypeCode=ObjectPtrProperty)

    public: ObjectPtrProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    public: virtual Object* Get (Object* target) const {return nullptr;}

    public: virtual void Set (Object* target, Object* value) {noz_assert(false);}

    public: virtual Type* GetObjectType (void) const = 0;

    public: virtual bool IsEqual (Object* t1, Object* t2) const override;

    public: virtual Property* CreateProxy (PropertyProxy* proxy) const override;

    public: virtual noz_int32 GetAnimationTrackCount (void) const {return 1;}

    public: virtual BlendTarget* CreateBlendTarget (Object* target) const override;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };

}

#endif // __noz_ObjectPtrProperty_h__

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Vector2Property_h__
#define __noz_Vector2Property_h__

namespace noz {

  class Vector2Property : public Property {
    NOZ_OBJECT(NoAllocator,TypeCode=Vector2Property)

    /// Default constructor
    public: Vector2Property(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Override to get the property value
    public: virtual const Vector2& Get (Object* target) const;

    /// Override to set the property value 
    public: virtual void Set (Object* target, const Vector2& value) = 0;

    /// Return true if the property value is the same within both targets
    public: virtual bool IsEqual (Object* t1, Object* t2) const override;

    public: virtual noz_int32 GetAnimationTrackCount(void) const override {return 2;}

    public: virtual const Name& GetAnimationTrackName(noz_int32 i) const override;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;

    public: virtual BlendTarget* CreateBlendTarget (Object* target) const override;
  };

}

#endif // __noz_Vector2Property_h__

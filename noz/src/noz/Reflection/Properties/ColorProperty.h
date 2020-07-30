///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ColorProperty_h__
#define __noz_ColorProperty_h__

namespace noz {

  class ColorProperty : public Property {
    NOZ_OBJECT(TypeCode=ColorProperty)

    /// Default constructor
    public: ColorProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Override to implement getting the property value in the target
    public: virtual Color Get(Object* t) const;

    /// Override to implement setting the property value in the target
    public: virtual void Set (Object* t, Color v) = 0;

    /// Return true if the property value is the same within both targets
    public: virtual bool IsEqual (Object* t1, Object* t2) const override;

    public: virtual Property* CreateProxy (PropertyProxy* proxy) const override;

    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;

    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;

    public: virtual noz_int32 GetAnimationTrackCount (void) const override {return 4;}

    public: virtual const Name& GetAnimationTrackName (noz_int32 i) const override;

    public: virtual BlendTarget* CreateBlendTarget (Object* target) const override;
  };
  
}

#endif // __noz_ColorProperty_h__

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_EnumProperty_h__
#define __noz_EnumProperty_h__

namespace noz {


  class EnumProperty : public Property {
    NOZ_OBJECT(TypeCode=EnumProperty)

    public: EnumProperty(PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) {}

    public: virtual Type* GetEnumType (void) const = 0;

    /// Set the value of the enum using a string.
    public: virtual void Set (Object* t, const Name& v) = 0;

    /// Get the value of the enum as a string.
    public: virtual const Name& Get (Object* t) const = 0;

    public: virtual const std::vector<Name>& GetNames (void) const = 0;

    public: virtual bool Serialize (Object* target, Serializer& serializer) override;

    public: virtual bool Deserialize (Object* target, Deserializer& serializer) override;

    public: virtual BlendTarget* CreateBlendTarget (Object* target) const override;

    public: virtual noz_int32 GetAnimationTrackCount (void) const {return 1;}
  };


  template <typename T> class EnumPropertyT : public EnumProperty {
    public: EnumPropertyT(PropertyAttributes attr=PropertyAttributes::Default) : EnumProperty(attr) {}
    
    /// Return the type definition for the enum
    public: virtual Type* GetEnumType(void) const override {return Enum<T>::type__;}

    public: virtual const std::vector<Name>& GetNames (void) const {return Enum<T>::GetNames();}

    public: virtual T GetRaw (Object* t) const = 0;
    
    public: virtual void SetRaw (Object* t, T v) = 0;

    public: virtual void Set (Object* t, const Name& v) {SetRaw(t,Enum<T>::GetValue(v));}

    public: virtual const Name& Get (Object* t) const {return Enum<T>::GetName(GetRaw(t));}

    /// Return true if the property value is the same within both targets
    public: virtual bool IsEqual (Object* t1, Object* t2) const override {
      if(t1==nullptr||t2==nullptr) return false;
      return Get(t1) == Get(t2);
    }
  };

} // namespace noz

#endif // __noz_EnumProperty_h__

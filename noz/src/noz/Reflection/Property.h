///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Property_h__
#define __noz_Property_h__

namespace noz {

  class Serializer;
  class Deserializer;
  class PropertyProxy;
  class KeyFrame;
  class BlendTarget;

  struct PropertyAttributes : public Attributes {
    PropertyAttributes(noz_uint32 value=0) : Attributes(value) {}

    static const noz_uint32 Serializable = NOZ_BIT(0);
    static const noz_uint32 Read         = NOZ_BIT(1);
    static const noz_uint32 Write        = NOZ_BIT(2);
    static const noz_uint32 Private      = NOZ_BIT(3);
    static const noz_uint32 ControlPart  = NOZ_BIT(4);

    static const noz_uint32 Default = Serializable|Read|Write;
  };

  class Property : public Object {
    NOZ_OBJECT()

    friend class Type;

    /// Name of the property (unique within type)
    private: Name name_;

    /// Property attributes
    private: PropertyAttributes attributes_;

    /// Type that the property belongs to
    private: Type* type_;

    /// Meta data for the type as found in NOZ_PROPERTY key value pairs
    private: std::map<Name,String> meta_;

    /// Initialize the property with the give attributes
    public: Property(PropertyAttributes attributes=PropertyAttributes::Default);

    /// Return true if the property is a control part
    public: bool IsControlPart (void) const {return (attributes_&PropertyAttributes::ControlPart)==PropertyAttributes::ControlPart;}

    /// Return true if the property is private
    public: bool IsPrivate (void) const {return (attributes_&PropertyAttributes::Private)==PropertyAttributes::Private;}

    /// Return true if the property is read only
    public: bool IsReadOnly(void) const {return !(attributes_&PropertyAttributes::Write);}

    /// Return true if the property can be serialized
    public: bool IsSerializable(void) const {return (attributes_&PropertyAttributes::Serializable)==PropertyAttributes::Serializable;}

    /// Return true if the property is write only
    public: bool IsWriteOnly(void) const {return !(attributes_&PropertyAttributes::Read);}

    /// Returns true if the property value in the target is the default value
    /// for the object type.
    public: virtual bool IsDefault (Object* target) const;

    /// Return the name of the property
    public: const Name& GetName(void) const {return name_;}

    /// Return the type that the property belongs to.
    public: Type* GetParentType(void) const {return type_;}

    /// Return the meta string for the given meta name or nullptr if the meta string does not exist
    public: const char* GetMeta (const Name& name, const char* def=nullptr) const;

    /// Set meta on the property
    public: void SetMeta (const Name& name, const char* value);

    /// Create a proxy property which uses the given object identifier to find the proxy object
    public: virtual Property* CreateProxy (PropertyProxy* proxy) const {return nullptr;}

    public: virtual bool IsEqual (Object* t1, Object* t2) const {return false;}

    public: virtual bool Serialize (Object* target, Serializer& serializer) {noz_assert(false); return false; }

    public: virtual bool Deserialize (Object* target, Deserializer& serializer) { noz_assert(false); return false; }

    /// Return the number of animation tracks for the property (Default is 0 which means the property is not animatable)
    public: virtual noz_int32 GetAnimationTrackCount (void) const {return 0;}

    /// Return the name of the animation channel at the given index
    public: virtual const Name& GetAnimationTrackName (noz_int32 i) const { return Name::Empty; }

    /// Create a blend target for the the given object..
    public: virtual BlendTarget* CreateBlendTarget (Object* target) const {return nullptr;}
  };

  template <typename T> T* Cast (Object* c) {
    if(c==nullptr) return nullptr;
    Type* type = c->GetType();
    noz_assert(type);
    return type->IsCastableTo(typeof(T)) ? (T*)c : nullptr;
  };

  template <typename T> T* Type::GetProperty(const Name& name) const {
    return Cast<T>(GetProperty(name));
  }

  template <typename T> T* Object::GetProperty(const Name& name) const {
    Property* p = GetProperty(name);
    if(nullptr == p || !p->IsTypeOf(typeof(T))) return nullptr;
    return (T*)p;
  }
}



#include "PropertyPath.h"
#include "PropertyProxy.h"

#endif // __noz_Property_h__

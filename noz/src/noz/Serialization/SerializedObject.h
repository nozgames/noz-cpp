///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SerializedObject_h__
#define __noz_SerializedObject_h__

namespace noz {

class SerializedObject : public Object {
  NOZ_OBJECT()

  NOZ_PROPERTY(Name=Type,Private)
  private: Type* type_;

  NOZ_PROPERTY(Name=Data,Private)
  private: std::vector<noz_byte> data_;

  public: SerializedObject (void);

  public: void Set (Object* o);

  public: noz_uint32 GetSize (void) const {return data_.size();}

  public: const noz_byte* GetBuffer (void) const {return &data_[0];}

  public: bool DeserializeInto (Object *o) const;

  public: Object* CreateInstance (void) const;

  public: template <typename T> T* CreateInstance (void) const {
    if(nullptr==type_ || !type_->IsCastableTo(typeof(T))) return nullptr;
    Object* o = CreateInstance();
    if(nullptr==o || !o->IsTypeOf(typeof(T))) {
      delete o;
      return nullptr;
    }
    return (T*)o;
  }
};

} // namespace noz


#endif // __noz_SerializedObject_h__



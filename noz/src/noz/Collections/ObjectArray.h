///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ObjectArray_h__
#define __noz_ObjectArray_h__

namespace noz {

  class ObjectArray : public Object {
    NOZ_OBJECT()

    /// Actual array of objects.
    NOZ_PROPERTY(Name=Objects)
    private: std::vector<ObjectPtr<Object>> objects_;

    /// Add an object to the array.
    public: void operator+= (Object* o);

    /// Return an object within the array at the given index
    public: Object* operator [] (noz_int32 index) const {return objects_[index];}

    /// Return the number of elements int the array
    public: noz_uint32 GetCount(void) const {return objects_.size();}

    /// Returns true if the array contains an object of the given type
    public: bool Contains (Type* t) const;
  };

} // namespace noz


#endif // __noz_ObjectArray_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ObjectPtrVectorProperty_h__
#define __noz_ObjectPtrVectorProperty_h__

namespace noz {

  class ObjectPtrVectorProperty : public Property {
    NOZ_OBJECT(TypeCode=ObjectPtrVectorProperty)

    /// Default constructor
    public: ObjectPtrVectorProperty (PropertyAttributes attr=PropertyAttributes::Default) : Property(attr) { }

    /// Returns the number of objects in the vector.
    public: virtual noz_uint32 GetCount (Object* target) const = 0;

    /// Reserves memory for the given amount to reduce allocations when objects are being added.  For 
    /// example if you knew you had ten items to add you could reserve space by calling Reserve with
    /// a value of ten which would limit the allocations within the vector to a single allocation.
    public: virtual void Reserve (Object* target, noz_uint32 capacity) = 0;

    /// Return the object at the given index.
    public: virtual Object* Get (Object* target, noz_uint32 index) const = 0;

    /// Add an object to the end of the vector and return the index that represents it
    public: virtual void Add (Object* target, Object* value) = 0;

    /// Remove the object at the given index and free any memory associated with the object.
    public: virtual void Remove (Object* target, noz_uint32 index) = 0;

    /// Insert the given object into the vector at the given index.  Any objects at 
    /// the given index will be moved up in the list.
    public: virtual void Insert (Object* target, noz_uint32 index, Object* value) = 0;

    /// Clear all objects from the list and free any associated memory
    public: virtual void Clear (Object* target) = 0;

    /// Releases the object at the given index by removing it from the list and 
    /// returning it for the caller to manage.  This method is used to transfer
    /// ownership of the object to the caller and remove it from the list.
    public: virtual Object* Release (Object* target, noz_uint32 index) = 0;

    /// Return the type of object contained within the vector
    public: virtual Type* GetObjectType (void) const = 0;    

    /// Proxy
    public: virtual Property* CreateProxy (PropertyProxy* proxy) const override;

    /// Serialization    
    protected: virtual bool Serialize (Object* target, Serializer& serializer) override;
    protected: virtual bool Deserialize (Object* target, Deserializer& serializer) override;
  };
}

#endif // __noz_ObjectPtrVectorProperty_h__

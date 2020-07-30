///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Object_h__
#define __noz_Object_h__


namespace noz {

  class String;
  class Type;
  class Object;
  
  template <typename T> class ObjectPtr;

  class ObjectManager {
    friend class Object;
    template <typename T> friend class ObjectPtr;

    private: struct Slot {
      noz_uint32 refs_;
      Object* object_;
    };

    private: static ObjectManager* this_;

    private: std::vector<Slot> slots_;

    private: noz_uint32 next_free_slot_;

    public: ObjectManager(void);

    public: static void Initialize (void);

    public: static void Uninitialize (void);

    public: static Object* GetObject(noz_uint32 slot_id) {return this_->slots_[slot_id].object_;}

    public: static noz_uint32 GetObjectCount (void) {return this_->slots_.size();}

    public: static noz_uint32 GetObjectRefs (noz_uint32 slot_id) {return this_->slots_[slot_id].refs_;}

    private: static noz_uint32 AllocSlot (Object* o);

    private: static void AddSlotRef (noz_uint32 slot);

    private: static void SetSlotObject (noz_uint32 slot_id, Object* o);

    private: static void ReleaseSlot (noz_uint32 slot);
  };

  class Object {
    NOZ_OBJECT_BASE(EditorIcon="{9A6D3EE8-AF34-4C6E-8C9B-1E60B8EFE070}")

    private: struct {
      /// Unique identifier of the object
      noz_uint32 id_:24;
    
      /// True if the object is currently being deserialize
      noz_uint32 deserializing_:1;
    };

    /// Default constructor
    public: Object (void);

    /// Overload the copy constructor to ensure id does not get copied
    public: Object (const Object& o);

    /// Virtual destructor
    public: virtual ~Object(void);

    public: bool IsDeserializing(void) const {return deserializing_;}

    /// Return the objects unique identifier 
    public: noz_uint32 GetId (void) const {return id_;}

    /// Return the number of references to this object. A value of one means the object has no weak references.
    public: noz_uint32 GetRefs (void) const {return ObjectManager::GetObjectRefs(id_);}    

    /// Return the typecode of the object
    public: TypeCode GetTypeCode(void) const {return GetType()->GetTypeCode();}

    /// Returns true if the object is the exact type
    public: bool IsType(Type* type) const {return GetType()==type;}

    /// Returns true if the object is the given type or any base or interface type.
    public: bool IsTypeOf(Type* type) const;

    /// Overload operator= to prevent id from being copied
    public: Object& operator= (const Object& o) {return *this;}

    /// Virtual method that returns the virtual object type.  The default version of
    /// this method returns the internal type which is a virtual method defined
    /// in the NOZ_OBJECT macro.  If you want an object instance to fake a type
    /// then override GetType and return your own type.
    public: virtual Type* GetType(void) const {return GetTypeInternal();}

    /// Convert the class to a string.
    public: virtual String ToString(void) const;

    /// Override to determine equality between arbitrary objects
    public: virtual bool Equals (Object* o) const {return this == o;}

    /// Create an instance of the given type 
    public: virtual Object* CreateInstance (Type* t) {return nullptr;}

    public: Property* GetProperty(const Name& name) const;

    public: template <typename T> T* GetProperty(const Name& name) const;

    /// Virtual method used by the PropertyProxy to return a Proxy object 
    /// for a given index.  If the given identifier is unknown then a value
    /// of nullptr will be returned.
    public: virtual Object* GetPropertyProxyObject (noz_uint32 id) const {return nullptr;}

    public: virtual Type* GetTypeInternal(void) const {return nullptr;}

    public: virtual void OnDeserializing (void);

    public: virtual void OnDeserialized (void);
  };

  template <typename T> class ObjectPtr {
    private: noz_uint32 id_;

    public: ObjectPtr(T* o) {
      id_ = 0;
      *this = o;
    }

    public: ObjectPtr(void) {
      id_ = 0;
    }

    public: ObjectPtr(const ObjectPtr& ptr) {
      id_ = 0;
      *this = ptr.GetObject();
    }

    public: ~ObjectPtr(void) {
      if(id_!=0) ObjectManager::ReleaseSlot(id_);
    }

    public: ObjectPtr& operator= (const ObjectPtr& ptr) {*this = ptr.GetObject(); return *this;}

    public: ObjectPtr& operator= (T* o) {
      noz_uint32 old_id = id_;
      if(o) {
        id_ = o->GetId();
        ObjectManager::AddSlotRef(id_);
      } else {
        id_ = 0;
      }
      ObjectManager::ReleaseSlot(old_id);
      return *this;
    }

    /// Member of pointer operator.
    public: T* operator -> (void) {return (T*)ObjectManager::GetObject(id_);}
    public: T* operator -> (void) const {return (T*)ObjectManager::GetObject(id_);}

    public: operator T* (void) const {return (T*)ObjectManager::GetObject(id_);}
    public: T* GetObject (void) const {return (T*)ObjectManager::GetObject(id_);}
  };

  template <typename T> class WeakPtr {
    private: noz_uint32 id_;

    public: WeakPtr(T* o) {
      id_ = 0;
      *this = o;
    }

    public: WeakPtr(void) {
      id_ = 0;
    }

    public: WeakPtr(const WeakPtr& ptr) {
      id_ = 0;
      *this = ptr.GetObject();
    }

    public: ~WeakPtr(void) {
      if(id_!=0) ObjectManager::ReleaseSlot(id_);
    }

    public: WeakPtr& operator= (const WeakPtr& ptr) {*this = ptr.GetObject(); return *this;}

    public: WeakPtr& operator= (T* o) {
      noz_uint32 old_id = id_;
      if(o) {
        id_ = o->GetId();
        ObjectManager::AddSlotRef(id_);
      } else {
        id_ = 0;
      }
      ObjectManager::ReleaseSlot(old_id);
      return *this;
    }

    /// Member of pointer operator.
    public: T* operator -> (void) {return (T*)ObjectManager::GetObject(id_);}
    public: T* operator -> (void) const {return (T*)ObjectManager::GetObject(id_);}

    public: operator T* (void) const {return (T*)ObjectManager::GetObject(id_);}
    public: T* GetObject (void) const {return (T*)ObjectManager::GetObject(id_);}
  };
}

#endif // __noz_Object_h__

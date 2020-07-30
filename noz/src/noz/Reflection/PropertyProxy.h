///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_PropertyProxy_h__
#define __noz_PropertyProxy_h__

namespace noz {

  class PropertyProxy : public Object { 
    NOZ_OBJECT()

    public: class Def : public Object {
      NOZ_OBJECT();

      /// Name of the export
      NOZ_PROPERTY(Name=Name)
      public: Name name_;

      /// Object being exported
      NOZ_PROPERTY(Name=Object)
      public: ObjectPtr<Object> exported_object_;

      /// Name of property within exported object being exported
      NOZ_PROPERTY(Name=Property)
      public: Name exported_property_;      
    };

    NOZ_PROPERTY(Name=Name)
    private: Name name_;

    /// Identifier of proxy object within target
    NOZ_PROPERTY(Name=ObjectId)
    private: noz_uint32 proxy_object_id_;

    /// Proxy property within target.
    NOZ_PROPERTY(Name=Property)
    private: Property* proxy_property_;

    /// Constructor
    public: PropertyProxy(void) : proxy_object_id_(0), proxy_property_(nullptr) {}
    public: PropertyProxy(const Name& name, noz_uint32 object_id, Property* prop) : name_(name), proxy_object_id_(object_id), proxy_property_(prop) {}

    /// Return the proxy property
    public: template <typename T> T* GetProxyProperty(void) const {
      return Cast<T>(proxy_property_);
    }

    /// Return the proxy property
    public: Property* GetProxyProperty(void) const {return proxy_property_;}

    /// Return the external property name
    public: const Name& GetName (void) const {return name_;}
  
    /// Return the proxy object within the given target 
    public: inline Object* GetProxyObject(Object* target) const {
      return target->GetPropertyProxyObject(proxy_object_id_);
    }
  };
}

#endif // __noz_PropertyProxy_h__

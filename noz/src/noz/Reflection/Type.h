///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Type_h__
#define __noz_Type_h__

namespace noz {
  enum class TypeCode : noz_byte {
    Boolean,        /// bool
    Byte,           /// noz_byte
    Float,          /// noz_float
    Double,         /// noz_double
    Int32,          /// noz_int32
    Int64,          /// noz_int64
    UInt32,         /// noz_uint32
    UInt64,         /// noz_uint64
    Class,          /// "class"
    Struct,         /// "struct"
    String,         /// StringObject
    Object,         /// Derived from Object class
    Asset,          /// Derived from Asset class
    Enum,           /// Enumeration
    Vector2,        /// Vector2Object
    Type,           /// Type Pointer
    Color,          /// ColorObject

    // Property classes
    BooleanProperty,
    ByteProperty,
    ByteVectorProperty,
    ChildrenProperty,
    ColorProperty,
    ComponentProperty,
    ComponentVectorProperty,
    EnumProperty,
    FloatProperty,          
    GuidProperty,
    Int32Property,
    LayoutLengthProperty,
    MethodProperty,
    NameProperty,
    NodePathProperty,
    ObjectProperty,
    ObjectPtrProperty,
    ObjectPtrVectorProperty,
    ObjectVectorProperty,
    PropertyPathProperty,
    PropertyProperty,
    RectProperty,
    StringProperty,
    TypeProperty,
    Vector2Property,
  }; 
}

namespace noz {
  
  class Object;
  class Property;
  class Method;
  class ObjectAllocator;

  struct TypeAttributes : public Attributes {
    TypeAttributes(noz_uint32 value=0) : Attributes(value) {}
    static const noz_uint32 Managed = NOZ_BIT(0);
    static const noz_uint32 EditorOnly = NOZ_BIT(1);
    static const noz_uint32 Default = 0;
  };

  struct ObjectAllocatorAttributes : public Attributes {
    ObjectAllocatorAttributes(noz_uint32 value=0) : Attributes(value) {}
    static const noz_uint32 NoDefaultTemplate = NOZ_BIT(0);
    static const noz_uint32 Default = 0;
  };

  class ObjectAllocator {
    public: virtual Object* CreateInstance(ObjectAllocatorAttributes attr=ObjectAllocatorAttributes::Default) {return nullptr;}
  };

  class Type {
    private: noz_uint64 child_mask_;
    private: noz_uint64 id_mask_;
    private: noz_uint64 id_;

    /// Allocator used to allocate objects of this type..
    private: ObjectAllocator* allocator_;

    /// Namespace of type
    private: Name namespace_;

    /// Qualified name of the type 
    private: Name qualified_name_;

    /// Name of type not including namespace
    private: Name name_;

    /// Base type.
    private: Type* base_;

    /// Attributes of type
    private: TypeAttributes attr_;

    /// Type code used to determine what type this represents
    private: TypeCode code_;    

    /// Mask of all properties defined by this type.
    private: noz_uint64 property_mask_;

    /// Dictionary of all properties for the given type.
    private: std::map<Name,Property*> property_map_;

    /// Vector of all properties 
    private: std::vector<Property*> properties_;

    private: std::vector<Method*> methods_;

    /// Dictionary of all methods registered for type
    private: std::map<Name,Method*> method_map_;

    /// Meta data for the type as found in NOZ_OBJECT key value pairs
    private: std::map<Name,String> meta_;

    /// Object containing all default property values.
    private: Object* defaults_;

    private: bool data_defined_type_;

#if defined(NOZ_EDITOR)
    private: Name editor_name_;
#endif

    public: Type(const char* name, TypeAttributes attr, TypeCode code, Type* base);

    public: ~Type(void);

    /// Register a type with with the type registry
    public: static void RegisterType (Type* type);

    /// Register a dynamic type by loading an asset which registers the type.  This is used
    /// by Prefabs to register the Prefab type that is linked to the prefab.
    public: static Type* RegisterTypeFromAsset (const Name& name);

    /// Unregister all types that have been previously registered.
    public: static void UnregisterAllTypes (void);

    /// Find a type that matches the given name exactly
    public: static Type* FindType (const Name& name);

    /// Get vector of all types
    public: static const std::vector<Type*>& GetTypes(void);

    public: static std::vector<Type*> GetTypes (Type* base, const char* contains);

    /// Return the object containing all of the default property values.
    public: Object* GetDefaults(void) const {return defaults_;}

    /// Returns true if the type has an allocator 
    public: bool HasAllocator (void) const {return allocator_!=nullptr;}

    /// Create an instance of the type..
    public: Object* CreateInstance(ObjectAllocatorAttributes attr=ObjectAllocatorAttributes::Default);

    /// Return the name of the type
    public: Name GetName(void) const {return name_;}

    /// Return the fully qualified name of the type
    public: Name GetQualifiedName(void) const {return qualified_name_;}

    /// Return the property matching the given name.
    public: Property* GetProperty(const Name& name) const;

    /// Return the property matching the given name as the requested property type.  If the 
    /// property is found but does not match the requested type then nullptr will be returned.
    public: template <typename T> T* GetProperty(const Name& name) const;

    // Return the method with the given name.
    public: Method* GetMethod(const Name& name);

    public: const std::vector<Method*>& GetMethods(void) const {return methods_;}

    /// Return the base type
    public: Type* GetBase(void) const {return base_;}

    /// Return true if the type is an object 
    public: bool IsObject(void) const {return code_==TypeCode::Object;}

    /// Return true if the type is an asset
    public: bool IsAsset(void) const {return code_==TypeCode::Asset;}

    public: bool IsEditorOnly (void) const {return !!(attr_ & TypeAttributes::EditorOnly);}

    /// Return true if the type is a managed asset
    public: bool IsManagedAsset(void) const {return IsAsset() && (attr_&TypeAttributes::Managed)==TypeAttributes::Managed;}

    /// Return the type code associated with the type
    public: TypeCode GetTypeCode(void) const {return code_; }

    /// Return properties dictionary
    public: const std::vector<Property*>& GetProperties(void) const {return properties_;}

    public: String GetMeta (const Name& name) const;

    /// Register a new property with the type
    public: bool RegisterProperty(Name name,Property* prop);

    public: bool RegisterMethod(Name name,Method* method);

    public: bool RegisterMeta (const Name& name, const char* value);

    public: bool IsCastableTo (Type* c) const {return (id_ & c->id_mask_) == c->id_;}
    
    /// Templated create instance
    public: template <typename T> T* CreateInstance(ObjectAllocatorAttributes attr=ObjectAllocatorAttributes::Default) {return (T*)CreateInstance();}

    /// Set the allocator for the type
    public: void SetAllocator(ObjectAllocator* allocator);   

    public: static void GenerateMasks (void);

#if defined(NOZ_EDITOR)
    public: const Name& GetEditorName (void) const {return editor_name_;}
#endif
  };
}

#endif // __noz_Type_h__

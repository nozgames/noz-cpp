///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Method_h__
#define __noz_Method_h__

namespace noz {

  struct MethodAttributes {
    noz_uint32 value;
    MethodAttributes(void) {}
    MethodAttributes(noz_uint32 _value) {value=_value;}
    
    operator noz_uint32 (void) const {return value;}
    
    static const noz_uint32 Default = 0;
  };

  class Method : public Object {
    NOZ_OBJECT(NoAllocator)
   
    friend class Type;

    private: MethodAttributes attr_;
    private: Name name_;
    private: Type* parent_type_;

    public: Method (MethodAttributes attr=MethodAttributes::Default);

    public: MethodAttributes GetAttributes(void) const {return attr_;}

    public: void SetName (const Name& n) {name_ = n;}

    public: const Name& GetName(void) const {return name_;}

    public: Type* GetParentType(void) const {return parent_type_;}

    /// Override to return the type that the target must be to invoke this method.
    public: virtual Type* GetTargetType(void) const = 0;

    /// Override to invoke the actual method.  It should be assumed that the target is
    /// the proper type needed by the method.
    public: virtual void Invoke (Object* target, noz_int32 argc, Object* argv[]) = 0;
  };

} // namespace noz


#endif //__noz_Int32_h__


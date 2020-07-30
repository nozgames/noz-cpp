///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Factory_h__
#define __noz_Factory_h__

namespace noz {

  namespace Editor {class FactoryFile;}

  class Factory : public Asset {    
    NOZ_OBJECT(Abstract,Managed)

    friend class Editor::FactoryFile;
    
    private: class ObjectDef : public Object {
      NOZ_OBJECT()

      NOZ_PROPERTY(Name=Property)
      public: Name property_;

      NOZ_PROPERTY(Name=Object)
      public: ObjectPtr<Object> object_;
    };

    private: class Def : public Object {
      NOZ_OBJECT()

      NOZ_PROPERTY(Name=FactoryType)
      public: Type* factory_type_;

      NOZ_PROPERTY(Name=Objects)
      public: std::vector<ObjectPtr<ObjectDef>> objects_;
    };

    protected: Factory (void);
  };

} // namespace noz


#endif //__noz_Factory_h__




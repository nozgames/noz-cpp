///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_EditorFactory_h__
#define __noz_Editor_EditorFactory_h__

namespace noz {
namespace Editor {

  class PropertyEditor;
  class Inspector;

  class EditorFactory {
    private: struct RegisteredPropertyEditor {
      Type* property_type_;
      Type* data_type_;
      Type* editor_type_;
    };

    private: struct RegisteredInspector {
      Type* target_type_;
      Type* inspector_type_;
    };


    private: static EditorFactory* this_;

    private: std::vector<RegisteredPropertyEditor> property_editors_;

    private: std::vector<RegisteredInspector> inspectors_;

    public: static void Initialize (void);

    public: static void Uninitialize (void);

    public: static PropertyEditor* CreatePropertyEditor (Property* p);

    public: static Sprite* CreateTypeIcon (Type* type);

    public: static Inspector* CreateInspector (Object* object);
    
    private: void RegisterPropertyEditors (void);

    private: void RegisterInspectors (void);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_EditorFactory_h__


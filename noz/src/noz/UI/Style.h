///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Style_h__
#define __noz_Style_h__

#include "StyleSheet.h"

namespace noz { namespace Editor {class StyleEditor;} }
namespace noz { namespace Editor {class StyleFile;} }
namespace noz { namespace Editor {class StyleInspector;} }

namespace noz {

  class Control;

  class Style : public Asset {
    NOZ_OBJECT(Managed,EditorIcon="{BF23FB4B-0C29-49E5-81F4-0C98B837EACE}")

    friend class Editor::StyleEditor;
    friend class Editor::StyleFile;
    friend class Editor::StyleInspector;
    friend class Control;

    private: class Template;

    private: class ControlPart : public Object {
      NOZ_OBJECT()

      friend class Style::Template;
      friend class Editor::StyleFile;
      friend class Editor::StyleEditor;
      friend class Editor::StyleInspector;

      /// Property within the control to set with the object
      NOZ_PROPERTY(Name=Property)
      private: Name property_;

      /// Object within the instance that represents the part
      NOZ_PROPERTY(Name=Object)
      private: ObjectPtr<Object> object_;
    };

    public: class Def : public Object {
      NOZ_OBJECT(EditorName="Style",EditorIcon="{BF23FB4B-0C29-49E5-81F4-0C98B837EACE}")

      /// Represents the base control type that the template extends from
      NOZ_PROPERTY(Name=ControlType,EditorFilterType=noz::Control)
      public: Type* control_type_;

      /// Nodes exported by template
      NOZ_PROPERTY(Name=Nodes)
      public: std::vector<ObjectPtr<Node>> nodes_;

      /// Vector of all defined parts
      NOZ_PROPERTY(Name=Parts)
      public: std::vector<ControlPart> parts_;

      public: Def(void) : control_type_(nullptr) {}
    };

    private: class Template : public Object {
      NOZ_OBJECT()

      friend class Editor::StyleFile;

      NOZ_PROPERTY(Name=Node,Deserialize=DeserializeNodes)
      private: std::vector<ObjectPtr<Node>> nodes_;

      NOZ_PROPERTY(Name=Parts,Deserialize=DeserializeParts,Serialize=SerializeParts)
      private: std::vector<ControlPart> parts_;

      /// Target control to use for deserialization.
      private: Control* target_;

      public: Template(void) : target_(nullptr) {}
      public: Template(Control* t) : target_(t) {}

      private: bool DeserializeNodes (Deserializer& s);
      private: bool DeserializeParts (Deserializer& s);

      private: bool SerializeParts (Serializer& s);
    };

    /// Control type to apply this style to
    NOZ_PROPERTY(Name=ControlType)
    private: Type* control_type_;

    /// Control Template 
    NOZ_PROPERTY(Name=Template,Private)
    private: SerializedObject template_;

    /// Default constructor
    public: Style (void);

    /// Default destructor
    public: ~Style (void);

    /// Return the control type that is used to match the style.
    public: Type* GetControlType (void) const {return control_type_;}
  };

} // namespace noz


#endif // __noz_Style_h__


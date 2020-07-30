///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_NodeInspector_h__
#define __noz_Editor_NodeInspector_h__

#include "../Inspector.h"
#include "../PropertyEditor/NamePropertyEditor.h"

namespace noz { class Expander; }

namespace noz {
namespace Editor {

  class TypePicker;

  class NodeInspector : public Inspector {
    NOZ_OBJECT(DefaultStyle="{97DEA3BC-CE69-4406-ADF3-FDFB6120620A}",EditorTarget=noz::Node)

    private: NOZ_CONTROL_PART(Name=NameEditor) ObjectPtr<NamePropertyEditor> name_editor_;
    private: NOZ_CONTROL_PART(Name=TypeText) ObjectPtr<TextNode> type_text_;
    private: NOZ_CONTROL_PART(Name=TypeIcon) ObjectPtr<SpriteNode> type_icon_;
    private: NOZ_CONTROL_PART(Name=AddComponentButton) ObjectPtr<Button> add_component_button_;
    private: NOZ_CONTROL_PART(Name=AddComponentPopup) ObjectPtr<Popup> add_component_popup_;
    private: NOZ_CONTROL_PART(Name=AddComponentTypePicker) ObjectPtr<TypePicker> add_component_type_picker_;
    private: NOZ_CONTROL_PART(Name=PropertiesContainer) ObjectPtr<Node> properties_container_;
    private: NOZ_CONTROL_PART(Name=PropertiesExpander) ObjectPtr<Expander> properties_expander_;

    public: NodeInspector (void);

    public: ~NodeInspector (void);

    protected: virtual bool OnApplyStyle (void) override;

    protected: virtual bool FilterProperty (Property* p) const override;

    private: void OnAddComponentButtonClicked (UINode* sender);
    private: void OnAddComponentTypeSelected (UINode* sender, Type* type);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_NodeInspector_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_ComponentInspector_h__
#define __noz_Editor_ComponentInspector_h__

#include "../Inspector.h"

namespace noz { class Expander; class Button; }

namespace noz {
namespace Editor {

  class TypePicker;

  class ComponentInspector : public Inspector {
    NOZ_OBJECT(DefaultStyle="{78AA1CB0-32DC-486F-8C8B-F81732DF9FB0}",EditorTarget=noz::Component)
    
    private: NOZ_CONTROL_PART(Name=Expander) ObjectPtr<Expander> expander_;
    private: NOZ_CONTROL_PART(Name=OptionsButton) ObjectPtr<Button> options_button_;
    private: NOZ_CONTROL_PART(Name=OptionsPopup) ObjectPtr<Popup> options_popup_;    
    private: NOZ_CONTROL_PART(Name=OptionsRemoveComponent) ObjectPtr<Button> options_remove_component_;    
    private: NOZ_CONTROL_PART(Name=PropertiesContainer) ObjectPtr<Node> properties_container_;

    public: ComponentInspector (void);

    public: ~ComponentInspector (void);

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnOptionsButtonClicked (UINode*);
    private: void OnOptionsRemoveComponent (UINode*);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_ComponentInspector_h__


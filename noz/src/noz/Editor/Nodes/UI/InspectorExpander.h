///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_InspectorExpander_h__
#define __noz_Editor_InspectorExpander_h__

#include <noz/Nodes/UI/Expander.h>
#include <noz/Nodes/Layout/StackLayout.h>
#include <noz/Nodes/Render/TextNode.h>

namespace noz {
namespace Editor {

  class InspectorEditor;

  class InspectorExpander : public Expander {
    NOZ_OBJECT(DefaultStyle="{C3DC422A-4CC9-46AF-A548-9DB1DC3CE8C3}")

    friend class InspectorEditor;

    public: InspectorExpander (void);

    public: ~InspectorExpander (void);
  };

  class ComponentExpander : public InspectorExpander {
    NOZ_OBJECT(DefaultStyle="{804BF423-F2B0-446A-B4A7-B2AF042973C6}");

    NOZ_CONTROL_PART(Name=OptionsButton)
    private: ObjectPtr<Button> options_button_;

    NOZ_CONTROL_PART(Name=Popup)
    private: ObjectPtr<Popup> popup_;

    NOZ_CONTROL_PART(Name=RemoveComponentButton)
    private: ObjectPtr<Button> remove_component_button_;

    private: ObjectPtr<Component> component_;

    public: ComponentExpander (Component* component) {
      component_ = component;
    }
    
    protected: virtual bool OnApplyStyle (void) override;

    private: void OnRemoveComponentButtonClicked (UINode* sender);
    private: void OnOptionsButtonClicked (UINode* sender);
  };


} // namespace Editor
} // namespace noz


#endif //__noz_Editor_InspectorExpander_h__


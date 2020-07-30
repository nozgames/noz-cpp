///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_StyleInspector_h__
#define __noz_Editor_StyleInspector_h__

#include "../Inspector.h"
#include "../PropertyEditor/TypePropertyEditor.h"

namespace noz {
namespace Editor {

  class TypePicker;

  class StyleInspector : public Inspector {
    NOZ_OBJECT(DefaultStyle="{5369441A-B371-406B-A906-2B532CC3E251}",EditorTarget=noz::Editor::StyleEditorRootNode)

    NOZ_CONTROL_PART(Name=ControlTypeEditor)
    private: ObjectPtr<TypePropertyEditor> control_type_editor_;
    
    NOZ_CONTROL_PART(Name=PartsContainer)
    private: ObjectPtr<Node> parts_container_;

    private: Style::Def* target_def_;

    public: StyleInspector (void);

    public: ~StyleInspector (void);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnSetTarget (Object* target) override;

    private: void RefreshControlParts (void);

    private: void OnControlTypeChanged (UINode* sender);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_StyleInspector_h__


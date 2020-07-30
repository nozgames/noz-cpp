///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_EventKeyFrameInspector_h__
#define __noz_Editor_EventKeyFrameInspector_h__

#include <noz/Editor/Nodes/UI/PropertyEditor/EnumPropertyEditor.h>
#include <noz/Editor/Nodes/UI/PropertyEditor/NamePropertyEditor.h>
#include <noz/Animation/EventKeyFrame.h>
#include "../Inspector.h"

namespace noz {
namespace Editor {

  class EventKeyFrameInspector : public Inspector {
    NOZ_OBJECT(DefaultStyle="{0A8DC503-30EE-42E8-B35F-908E8050C263}",EditorTarget=noz::EventKeyFrame)    

    private: NOZ_CONTROL_PART(Name=EventTypeEditor) ObjectPtr<EnumPropertyEditor> event_type_editor_;
    private: NOZ_CONTROL_PART(Name=ParamEditor) ObjectPtr<NamePropertyEditor> param_editor_;

    private: EventKeyFrame* key_frame_;

    public: EventKeyFrameInspector (void);

    public: ~EventKeyFrameInspector (void);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnSetTarget (Object* target) override;
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_EventKeyFrameInspector_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_ColorPropertyEditor_h__
#define __noz_Editor_ColorPropertyEditor_h__

#include <noz/Nodes/UI/ColorPicker.h>
#include "PropertyEditor.h"

namespace noz {
namespace Editor {

  class ColorPropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{18D108FC-3E1B-416D-9A25-3BC64843EF6E}",EditorProperty=noz::ColorProperty)

    NOZ_CONTROL_PART(Name=ColorPicker)
    private: ObjectPtr<ColorPicker> color_picker_;

    private: bool ignore_color_changes_;

    public: ColorPropertyEditor (void);

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnColorPickerColorChanged (UINode* sender);

    protected: virtual void Update (void) override;
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_ColorPropertyEditor_h__


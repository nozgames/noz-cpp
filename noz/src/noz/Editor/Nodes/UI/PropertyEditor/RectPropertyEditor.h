///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_RectPropertyEditor_h__
#define __noz_Editor_RectPropertyEditor_h__

#include "PropertyEditor.h"

namespace noz { class TextBox; }

namespace noz {
namespace Editor {

  class RectPropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{54980B8E-34B0-436C-B8B0-6C3E2E977098}",EditorProperty=noz::RectProperty)

    NOZ_CONTROL_PART(Name=TextBoxX)
    private: ObjectPtr<TextBox> text_box_x_;

    NOZ_CONTROL_PART(Name=TextBoxY)
    private: ObjectPtr<TextBox> text_box_y_;

    NOZ_CONTROL_PART(Name=TextBoxW)
    private: ObjectPtr<TextBox> text_box_w_;

    NOZ_CONTROL_PART(Name=TextBoxH)
    private: ObjectPtr<TextBox> text_box_h_;

    public: RectPropertyEditor (void);

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;

    protected: void OnTextBoxTextChanged (UINode*);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_RectPropertyEditor_h__


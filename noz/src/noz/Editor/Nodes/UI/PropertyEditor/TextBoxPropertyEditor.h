///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_TextBoxPropertyEditor_h__
#define __noz_Editor_TextBoxPropertyEditor_h__

#include "PropertyEditor.h"

namespace noz { class TextBox; }

namespace noz {
namespace Editor {

  class TextBoxPropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{4B57A203-0297-49F3-9CD2-35812848BB68}",Abstract)

    NOZ_CONTROL_PART(Name=TextBox)
    private: ObjectPtr<TextBox> text_box_;

    public: TextBoxPropertyEditor (void);

    protected: virtual String ReadString (Object* target, Property* prop) = 0;

    protected: virtual void WriteString (Object* target, Property* prop, const String& value) = 0;

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;

    protected: void OnTextBoxTextChanged (UINode*);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_TextBoxPropertyEditor_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_LayoutLengthPropertyEditor_h__
#define __noz_Editor_LayoutLengthPropertyEditor_h__

#include "PropertyEditor.h"

namespace noz { class TextBox; }

namespace noz {
namespace Editor {

  class LayoutLengthPropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{673A757E-5441-4F83-919D-EA13B749A7C6}",EditorProperty=noz::LayoutLengthProperty)

    NOZ_CONTROL_PART(Name=Text)
    private: ObjectPtr<TextBox> text_;

    NOZ_CONTROL_PART(Name=AutoButton)
    private: ObjectPtr<Button> auto_button_;

    public: LayoutLengthPropertyEditor (void);

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnTextBoxTextChanged (UINode*);
    private: void OnAutoButtonClicked (UINode*);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_LayoutLengthPropertyEditor_h__


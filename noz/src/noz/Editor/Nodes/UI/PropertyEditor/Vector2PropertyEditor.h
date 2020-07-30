///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_Vector2PropertyEditor_h__
#define __noz_Editor_Vector2PropertyEditor_h__

#include "PropertyEditor.h"

namespace noz { class TextBox; }

namespace noz {
namespace Editor {

  class Vector2PropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{A71B621B-30AD-4B6A-8572-687633CC50F9}",EditorProperty=noz::Vector2Property)

    NOZ_CONTROL_PART(Name=TextBoxX)
    private: ObjectPtr<TextBox> text_box_x_;

    NOZ_CONTROL_PART(Name=TextBoxY)
    private: ObjectPtr<TextBox> text_box_y_;

    private: Vector2 value_;

    public: Vector2PropertyEditor (void);

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void Update (void) override;

    protected: void OnTextBoxTextChanged (UINode*);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_Vector2PropertyEditor_h__


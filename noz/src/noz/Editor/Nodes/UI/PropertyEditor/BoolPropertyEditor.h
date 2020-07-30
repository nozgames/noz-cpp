///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_BoolPropertyEditor_h__
#define __noz_Editor_BoolPropertyEditor_h__

#include "PropertyEditor.h"

namespace noz { class CheckBox; }

namespace noz {
namespace Editor {

  class BoolPropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{B83F640E-E516-417F-9A15-C6CBA76C6655}",EditorProperty=noz::BooleanProperty)

    NOZ_CONTROL_PART(Name=CheckBox)
    private: ObjectPtr<CheckBox> check_box_;

    public: BoolPropertyEditor (void);

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnCheckBoxClick (UINode* sender);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_BoolPropertyEditor_h__


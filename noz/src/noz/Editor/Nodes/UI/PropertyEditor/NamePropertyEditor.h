///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_NamePropertyEditor_h__
#define __noz_Editor_NamePropertyEditor_h__

#include "TextBoxPropertyEditor.h"

namespace noz {
namespace Editor {

  class NamePropertyEditor : public TextBoxPropertyEditor {
    NOZ_OBJECT(DefaultStyle="{4B57A203-0297-49F3-9CD2-35812848BB68}",EditorProperty=noz::NameProperty)

    protected: virtual String ReadString (Object* target, Property* prop) override;

    protected: virtual void WriteString (Object* target, Property* prop, const String& value) override;
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_NamePropertyEditor_h__


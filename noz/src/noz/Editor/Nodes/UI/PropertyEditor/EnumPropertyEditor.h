///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_EnumPropertyEditor_h__
#define __noz_Editor_EnumPropertyEditor_h__

#include <noz/Nodes/UI/DropDownList.h>
#include "PropertyEditor.h"

namespace noz {
namespace Editor {

  class EnumPropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{E8D86A17-FA57-41EE-8001-26C1DE9CA151}",EditorProperty=noz::EnumProperty)

    NOZ_CONTROL_PART(Name=DropDownList)
    private: ObjectPtr<DropDownList> drop_down_list_;

    public: EnumPropertyEditor (void);

    public: const Name& GetValue (void) const;

    public: void SetValue (const Name& v);

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnSetTarget (void) override;
    protected: virtual void Update (void) override;

    private: void OnDropDownListSelectionChanged (UINode* sender);

    private: void UpdateDropDownList (void);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_EnumPropertyEditor_h__



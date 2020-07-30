///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_TypePropertyEditor_h__
#define __noz_Editor_TypePropertyEditor_h__

#include <noz/Nodes/UI/Button.h>
#include "PropertyEditor.h"
#include "../AssetPicker.h"

namespace noz {
namespace Editor {

  class TypePropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{5696BA49-5906-43C0-86CF-B77471E752B6}",EditorProperty=noz::TypeProperty)

    NOZ_CONTROL_PART(Name=Button)
    private: ObjectPtr<Button> button_;

    NOZ_CONTROL_PART(Name=Popup)
    private: ObjectPtr<Popup> popup_;

    NOZ_CONTROL_PART(Name=TypePicker)
    private: ObjectPtr<TypePicker> type_picker_;

    private: Type* type_;

    public: TypePropertyEditor (void);

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnButtonClick (UINode* sender);

    private: void OnTypeSelected (UINode* sender, Type* type);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_TypePropertyEditor_h__


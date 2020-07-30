///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_ControlPartPropertyEditor_h__
#define __noz_Editor_ControlPartPropertyEditor_h__

#include "ObjectPtrPropertyEditor.h"

namespace noz {
namespace Editor {

  class ControlPartPropertyEditor : public ObjectPtrPropertyEditor {
    NOZ_OBJECT(DefaultStyle="{6E486C2B-EAF8-4B38-BA98-32819F9026CA}",EditorProperty=noz::ObjectPtrProperty)

    private: Type* base_type_;

    public: ControlPartPropertyEditor (Type* base_type);

    protected: virtual Type* GetValueType (void) const {return base_type_;}
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_ControlPartPropertyEditor_h__


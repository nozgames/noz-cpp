///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_ObjectPtrPropertyEditor_h__
#define __noz_Editor_ObjectPtrPropertyEditor_h__

#include "PropertyEditor.h"

namespace noz {
namespace Editor {

  class ObjectPtrPropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{6E486C2B-EAF8-4B38-BA98-32819F9026CA}",EditorProperty=noz::ObjectPtrProperty)

    NOZ_CONTROL_PART(Name=TextNode)
    private: ObjectPtr<TextNode> text_node_;

    NOZ_CONTROL_PART(Name=SpriteNode)
    private: ObjectPtr<SpriteNode> sprite_node_;

    private: ObjectPtr<Object> value_;

    public: ObjectPtrPropertyEditor (void);

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnDragDrop (DragDropEventArgs* args) override;
    protected: virtual void OnSetTarget (void) override;
    protected: virtual void Update (void) override;

    protected: void SetObject (Object* object);

    protected: bool GetValueFromArgs (DragDropEventArgs* args, Object** value=nullptr) const;

    protected: virtual Type* GetValueType (void) const;

    private: void OnAssetRenamed (AssetFile* asset);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_ObjectPtrPropertyEditor_h__


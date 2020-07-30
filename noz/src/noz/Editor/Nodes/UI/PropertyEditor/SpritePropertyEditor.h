///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_SpritePropertyEditor_h__
#define __noz_Editor_SpritePropertyEditor_h__

#include <noz/Nodes/UI/Button.h>
#include "PropertyEditor.h"
#include "../AssetPicker.h"

namespace noz {
namespace Editor {

  class SpritePropertyEditor : public PropertyEditor {
    NOZ_OBJECT(DefaultStyle="{B69B2AC6-A63F-487E-A282-DB8839711E5B}",EditorProperty=noz::ObjectPtrProperty,EditorData=noz::Sprite)

    NOZ_CONTROL_PART(Name=Button)
    private: ObjectPtr<Button> button_;

    NOZ_CONTROL_PART(Name=Popup)
    private: ObjectPtr<Popup> popup_;

    NOZ_CONTROL_PART(Name=AssetPicker)
    private: ObjectPtr<AssetPicker> asset_picker_;

    NOZ_CONTROL_PART(Name=SpriteNode)
    private: ObjectPtr<SpriteNode> sprite_node_;

    NOZ_CONTROL_PART(Name=NoneNode)
    private: ObjectPtr<Node> none_node_;

    public: SpritePropertyEditor (void);

    protected: virtual void WriteProperty (Object* target, Property* prop) final;

    protected: virtual void ReadProperty (Object* target, Property* prop) final;

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void UpdateAnimationState (void) override;
    protected: virtual void Update (void) override;

    private: void OnButtonClick (UINode* sender);
    private: void OnAssetSelected (UINode* sender, Asset* asset);
    private: void OnButtonDragDrop (UINode* sender, DragDropEventArgs* args);

    private: void SetSprite (Sprite* sprite);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_SpritePropertyEditor_h__


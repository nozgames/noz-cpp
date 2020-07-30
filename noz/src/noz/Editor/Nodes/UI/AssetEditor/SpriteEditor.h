///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_SpriteEditor_h__
#define __noz_Editor_SpriteEditor_h__

#include <noz/Nodes/Render/ImageNode.h>
#include <noz/Nodes/UI/MeasureScaler.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/Popup.h>
#include <noz/Nodes/UI/DropDownList.h>
#include <noz/Editor/Nodes/Render/GridNode.h>
#include "AssetEditor.h"

namespace noz {
namespace Editor {

  class Memento;

  class SpriteEditor : public AssetEditor {
    NOZ_OBJECT(DefaultStyle="{CDA7101A-F665-4C3E-B78F-9171473E2799}",AssetType=noz::Sprite)

    private: NOZ_CONTROL_PART(Name=ImageNode) ObjectPtr<ImageNode> image_node_;
    private: NOZ_CONTROL_PART(Name=ZoomNode) ObjectPtr<MeasureScaler> zoom_node_;
    private: NOZ_CONTROL_PART(Name=GridNode) ObjectPtr<GridNode> grid_node_;
    private: NOZ_CONTROL_PART(Name=ZoomDropDownList) ObjectPtr<DropDownList> zoom_drop_down_list_;
    private: NOZ_CONTROL_PART(Name=WidthText) ObjectPtr<TextNode> width_text_;
    private: NOZ_CONTROL_PART(Name=HeightText) ObjectPtr<TextNode> height_text_;
    private: NOZ_CONTROL_PART(Name=SelectionTool) ObjectPtr<ToggleButton> selection_tool_;
    private: NOZ_CONTROL_PART(Name=MagicWandTool) ObjectPtr<ToggleButton> magic_wand_tool_;
    private: NOZ_CONTROL_PART(Name=Manipulator) ObjectPtr<Node> manipulator_;
    private: NOZ_CONTROL_PART(Name=ScrollView) ObjectPtr<ScrollView> scroll_view_;

    private: noz_float zoom_;

    private: ObjectPtr<Sprite> sprite_;

    public: SpriteEditor (void);

    public: ~SpriteEditor (void);

    protected: bool Load (AssetFile* file);

    public: virtual void Save (void) override;

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnMouseWheel (SystemEvent* e) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void Update (void) override;

    public: void SetZoom (noz_float zoom);

    protected: void OnPropertyChanged (PropertyChangedEventArgs* args);

    private: void UpdateDimensions (void);
    private: void UpdateManipulator (void);
    private: void OnSelectToolClick (UINode* sender);
    private: void OnMagicWandToolClick (UINode* sender);

    private: void MagicWand (const Vector2& uv);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_SpriteEditor_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_FontEditor_h__
#define __noz_Editor_FontEditor_h__

#include <noz/Nodes/Render/ImageNode.h>
#include <noz/Nodes/UI/MeasureScaler.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/Popup.h>
#include <noz/Nodes/UI/DropDownList.h>
#include <noz/Editor/Nodes/Render/GridNode.h>
#include "AssetEditor.h"
#include <noz/Editor/Asset/FontFile.h>

namespace noz {
namespace Editor {

  class Memento;

  class FontEditor : public AssetEditor {
    NOZ_OBJECT(DefaultStyle="{923D0242-0816-4865-9966-DB4C3782976B}",AssetType=noz::Font)

    private: NOZ_CONTROL_PART(Name=ImageNode) ObjectPtr<ImageNode> image_node_;
    private: NOZ_CONTROL_PART(Name=ZoomNode) ObjectPtr<MeasureScaler> zoom_node_;
    private: NOZ_CONTROL_PART(Name=GridNode) ObjectPtr<GridNode> grid_node_;
    private: NOZ_CONTROL_PART(Name=ZoomDropDownList) ObjectPtr<DropDownList> zoom_drop_down_list_;
    private: NOZ_CONTROL_PART(Name=WidthText) ObjectPtr<TextNode> width_text_;
    private: NOZ_CONTROL_PART(Name=HeightText) ObjectPtr<TextNode> height_text_;

    private: ObjectPtr<Inspector> inspector_;

    private: noz_float zoom_;

    private: ObjectPtr<Font> font_;

    private: FontDef font_def_;

    public: FontEditor (void);

    public: ~FontEditor (void);

    protected: bool Load (AssetFile* file);
    
    public: virtual void Save (void) override;

    protected: virtual bool OnApplyStyle (void) override;
    
    protected: virtual void OnMouseWheel (SystemEvent* e) override;

    public: void SetZoom (noz_float zoom);

    protected: void OnPropertyChanged (PropertyChangedEventArgs* args);

    private: void UpdateDimensions (void);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_FontEditor_h__


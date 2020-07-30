///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_ImageEditor_h__
#define __noz_Editor_ImageEditor_h__

#include <noz/Nodes/Render/ImageNode.h>
#include <noz/Nodes/UI/MeasureScaler.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/Popup.h>
#include <noz/Editor/Nodes/Render/GridNode.h>
#include <noz/Editor/Asset/ImageFile.h>
#include "AssetEditor.h"

namespace noz {
namespace Editor {

  class Memento;

  class ImageEditor : public AssetEditor {
    NOZ_OBJECT(DefaultStyle="{F4BC2792-5ABB-48B8-B2C1-C7A231510639}",AssetType=noz::Image)

    private: NOZ_CONTROL_PART(Name=WidthText) ObjectPtr<TextNode> width_text_;
    private: NOZ_CONTROL_PART(Name=HeightText) ObjectPtr<TextNode> height_text_;
    private: NOZ_CONTROL_PART(Name=ImageNode) ObjectPtr<ImageNode> image_node_;
    private: NOZ_CONTROL_PART(Name=ZoomNode) ObjectPtr<MeasureScaler> zoom_node_;
    private: NOZ_CONTROL_PART(Name=GridNode) ObjectPtr<GridNode> grid_node_;

    NOZ_CONTROL_PART(Name=ZoomButton)
    private: ObjectPtr<Button> zoom_button_;

    NOZ_CONTROL_PART(Name=ZoomPopup)
    private: ObjectPtr<Popup> zoom_popup_;

    NOZ_CONTROL_PART(Name=ZoomButton20)
    private: ObjectPtr<Button> zoom_button_20_;

    NOZ_CONTROL_PART(Name=ZoomButton50)
    private: ObjectPtr<Button> zoom_button_50_;

    NOZ_CONTROL_PART(Name=ZoomButton70)
    private: ObjectPtr<Button> zoom_button_70_;

    NOZ_CONTROL_PART(Name=ZoomButton100)
    private: ObjectPtr<Button> zoom_button_100_;

    NOZ_CONTROL_PART(Name=ZoomButton150)
    private: ObjectPtr<Button> zoom_button_150_;

    NOZ_CONTROL_PART(Name=ZoomButton200)
    private: ObjectPtr<Button> zoom_button_200_;

    NOZ_CONTROL_PART(Name=ZoomButton400)
    private: ObjectPtr<Button> zoom_button_400_;

    private: ObjectPtr<Inspector> inspector_;

    private: noz_float zoom_;

    private: ObjectPtr<Image> image_;

    private: ImageDef image_def_;

    public: ImageEditor (void);

    public: ~ImageEditor (void);

    protected: virtual bool Load (AssetFile* file) override;
    protected: virtual void Save (void) override;

    protected: virtual bool OnApplyStyle (void) override;
    
    protected: virtual void OnMouseWheel (SystemEvent* e) override;

    private: void UpdateDimensions (void);
    private: void OnZoomButton (UINode* sender);
    private: void OnZoomButton20 (UINode* sender);    
    private: void OnZoomButton50 (UINode* sender);
    private: void OnZoomButton70 (UINode* sender);
    private: void OnZoomButton100 (UINode* sender);
    private: void OnZoomButton150 (UINode* sender);
    private: void OnZoomButton200 (UINode* sender);
    private: void OnZoomButton400 (UINode* sender);
    private: void OnPropertyChanged (PropertyChangedEventArgs* args);

    public: void SetZoom (noz_float zoom);

  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_ImageEditor_h__


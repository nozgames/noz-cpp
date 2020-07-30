///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_LayoutTransformInspector_h__
#define __noz_Editor_LayoutTransformInspector_h__

#include "ComponentInspector.h"

namespace noz {
namespace Editor {

  class TypePicker;
  class LayoutLengthPropertyEditor;
  class FloatPropertyEditor;
  class Vector2PropertyEditor;
  class BoolPropertyEditor;

  class LayoutTransformInspector : public ComponentInspector {
    NOZ_OBJECT(DefaultStyle="{D1B78854-6765-4224-A8D2-F0BE7FB64F7C}",EditorTarget=noz::LayoutTransform)

    private: NOZ_CONTROL_PART(Name=WidthEditor) ObjectPtr<LayoutLengthPropertyEditor> width_editor_;
    private: NOZ_CONTROL_PART(Name=HeightEditor) ObjectPtr<LayoutLengthPropertyEditor> height_editor_;
    private: NOZ_CONTROL_PART(Name=MarginLEditor) ObjectPtr<LayoutLengthPropertyEditor> margin_l_editor_;
    private: NOZ_CONTROL_PART(Name=MarginREditor) ObjectPtr<LayoutLengthPropertyEditor> margin_r_editor_;
    private: NOZ_CONTROL_PART(Name=MarginTEditor) ObjectPtr<LayoutLengthPropertyEditor> margin_t_editor_;
    private: NOZ_CONTROL_PART(Name=MarginBEditor) ObjectPtr<LayoutLengthPropertyEditor> margin_b_editor_;
    private: NOZ_CONTROL_PART(Name=PaddingLEditor) ObjectPtr<FloatPropertyEditor> padding_l_editor_;
    private: NOZ_CONTROL_PART(Name=PaddingREditor) ObjectPtr<FloatPropertyEditor> padding_r_editor_;
    private: NOZ_CONTROL_PART(Name=PaddingTEditor) ObjectPtr<FloatPropertyEditor> padding_t_editor_;
    private: NOZ_CONTROL_PART(Name=PaddingBEditor) ObjectPtr<FloatPropertyEditor> padding_b_editor_;
    private: NOZ_CONTROL_PART(Name=ScaleEditor) ObjectPtr<Vector2PropertyEditor> scale_editor_;
    private: NOZ_CONTROL_PART(Name=PivotEditor) ObjectPtr<Vector2PropertyEditor> pivot_editor_;
    private: NOZ_CONTROL_PART(Name=RotationEditor) ObjectPtr<FloatPropertyEditor> rotation_editor_;
    private: NOZ_CONTROL_PART(Name=MinWidth) ObjectPtr<FloatPropertyEditor> min_width_editor_;
    private: NOZ_CONTROL_PART(Name=MaxWidth) ObjectPtr<FloatPropertyEditor> max_width_editor_;
    private: NOZ_CONTROL_PART(Name=MinHeight) ObjectPtr<FloatPropertyEditor> min_height_editor_;
    private: NOZ_CONTROL_PART(Name=MaxHeight) ObjectPtr<FloatPropertyEditor> max_height_editor_;
    private: NOZ_CONTROL_PART(Name=MaintainAspectRatioEditor) ObjectPtr<BoolPropertyEditor> maintain_aspect_ratio_editor_;
    private: NOZ_CONTROL_PART(Name=AspectRatioEditor) ObjectPtr<Vector2PropertyEditor> aspect_ratio_editor_;
    private: NOZ_CONTROL_PART(Name=Advanced) ObjectPtr<Expander> advanced_;

    public: LayoutTransformInspector (void);

    public: ~LayoutTransformInspector (void);

    protected: virtual bool OnApplyStyle (void) override;

    protected: virtual bool FilterProperty (Property* p) const override;
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_LayoutTransformInspector_h__


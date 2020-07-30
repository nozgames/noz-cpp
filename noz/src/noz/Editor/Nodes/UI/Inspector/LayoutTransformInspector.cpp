///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Components/Transform/LayoutTransform.h>
#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/Expander.h>
#include <noz/Editor/Nodes/UI/PropertyEditor/LayoutLengthPropertyEditor.h>
#include <noz/Editor/Nodes/UI/PropertyEditor/FloatPropertyEditor.h>
#include <noz/Editor/Nodes/UI/PropertyEditor/BoolPropertyEditor.h>
#include <noz/Editor/Nodes/UI/PropertyEditor/Vector2PropertyEditor.h>
#include "LayoutTransformInspector.h"

using namespace noz;
using namespace noz::Editor;


LayoutTransformInspector::LayoutTransformInspector(void) {
}

LayoutTransformInspector::~LayoutTransformInspector(void) {
}

bool LayoutTransformInspector::FilterProperty (Property* p) const {
  return false;
}

bool LayoutTransformInspector::OnApplyStyle (void) {
  if(!ComponentInspector::OnApplyStyle()) return false;

  if(width_editor_) width_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("Width"));
  if(height_editor_) height_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("Height"));
  if(margin_l_editor_) margin_l_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("MarginLeft"));
  if(margin_r_editor_) margin_r_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("MarginRight"));
  if(margin_t_editor_) margin_t_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("MarginTop"));
  if(margin_b_editor_) margin_b_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("MarginBottom"));
  if(padding_l_editor_) padding_l_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("PaddingLeft"));
  if(padding_r_editor_) padding_r_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("PaddingRight"));
  if(padding_t_editor_) padding_t_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("PaddingTop"));
  if(padding_b_editor_) padding_b_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("PaddingBottom"));
  if(scale_editor_) scale_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("Scale"));
  if(rotation_editor_) rotation_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("Rotation"));
  if(pivot_editor_) pivot_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("Pivot"));
  if(min_width_editor_) min_width_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("MinWidth"));
  if(max_width_editor_) max_width_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("MaxWidth"));
  if(min_height_editor_) min_height_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("MinHeight"));
  if(max_height_editor_) max_height_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("MaxHeight"));
  if(maintain_aspect_ratio_editor_) maintain_aspect_ratio_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("MaintainAspectRatio"));
  if(aspect_ratio_editor_) aspect_ratio_editor_->SetTarget(GetTarget(), GetTarget()->GetProperty("AspectRatio"));

  LayoutTransform* t = Cast<LayoutTransform>(GetTarget());
  if(advanced_ && t) {
    if(t->GetMinWidth()!=0.0f ||
       t->GetMinHeight()!=0.0f ||
       t->GetMaxWidth()!=Float::Infinity ||
       t->GetMaxHeight()!=Float::Infinity ||
       t->IsMaintainAspectRatio() == true ){
      advanced_->SetExpanded(true);
    }
  }

  return true;
}


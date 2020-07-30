///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_LayoutTransform_h__
#define __noz_LayoutTransform_h__

#include "Transform.h"

namespace noz {

  NOZ_ENUM() enum class LayoutUnitType {
    Fixed,
    Percentage,
    Auto
  };

  class LayoutLength {
    public: static LayoutLength Empty;

    public: LayoutUnitType unit_type_;
    public: noz_float value_;

    public: LayoutLength(void) : unit_type_(LayoutUnitType::Fixed), value_(0.0f) {}
    public: LayoutLength(noz_float v) : unit_type_(LayoutUnitType::Fixed), value_(v) {}
    public: LayoutLength(LayoutUnitType u, noz_float v) : unit_type_(u), value_(v) {}

    public: operator noz_float (void) const {return value_;}

    public: bool operator == (const LayoutLength& v) const {
      return v.unit_type_ == unit_type_ && (v.unit_type_==LayoutUnitType::Auto || v.value_ == value_);
    }

    public: bool IsAuto (void) const {return unit_type_ == LayoutUnitType::Auto;}
    public: bool IsFixed (void) const {return unit_type_ == LayoutUnitType::Fixed;}
    public: bool IsPercentage (void) const {return unit_type_ == LayoutUnitType::Percentage;}
  };

  class LayoutTransform : public Transform {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Margin,Type=LayoutLength,Set=SetMargin,WriteOnly)
    NOZ_PROPERTY(Name=Padding,Type=noz_float,Set=SetPadding,WriteOnly)

    NOZ_PROPERTY(Name=Width,Set=SetWidth)
    protected: LayoutLength width_;

    NOZ_PROPERTY(Name=Height,Set=SetHeight)
    protected: LayoutLength height_;


    NOZ_PROPERTY(Name=MarginLeft,Set=SetMarginLeft)
    protected: LayoutLength margin_left_;

    NOZ_PROPERTY(Name=MarginRight,Set=SetMarginRight)
    protected: LayoutLength margin_right_;

    NOZ_PROPERTY(Name=MarginTop,Set=SetMarginTop)
    protected: LayoutLength margin_top_;

    NOZ_PROPERTY(Name=MarginBottom,Set=SetMarginBottom)
    protected: LayoutLength margin_bottom_;


    NOZ_PROPERTY(Name=PaddingLeft,Set=SetPaddingLeft)
    protected: noz_float padding_left_;

    NOZ_PROPERTY(Name=PaddingRight,Set=SetPaddingRight)
    protected: noz_float padding_right_;

    NOZ_PROPERTY(Name=PaddingTop,Set=SetPaddingTop)
    protected: noz_float padding_top_;

    NOZ_PROPERTY(Name=PaddingBottom,Set=SetPaddingBottom)
    protected: noz_float padding_bottom_;


    NOZ_PROPERTY(Name=MinWidth,Set=SetMinWidth)
    protected: noz_float min_width_;

    NOZ_PROPERTY(Name=MaxWidth,Set=SetMaxWidth)
    protected: noz_float max_width_;

    NOZ_PROPERTY(Name=MinHeight,Set=SetMinHeight)
    protected: noz_float min_height_;

    NOZ_PROPERTY(Name=MaxHeight,Set=SetMaxHeight)
    protected: noz_float max_height_;

    NOZ_PROPERTY(Name=MaintainAspectRatio,Set=SetMaintainAspectRatio)
    protected: bool maintain_aspect_ratio_;

    NOZ_PROPERTY(Name=AspectRatio,Set=SetAspectRatio)
    protected: Vector2 aspect_ratio_;


    public: LayoutTransform (void);

    public: const LayoutLength& GetWidth(void) const {return width_;}
    public: const LayoutLength& GetHeight(void) const {return height_;}
    public: const LayoutLength& GetMarginLeft(void) const { return margin_left_; }
    public: const LayoutLength& GetMarginRight(void) const { return margin_right_; }
    public: const LayoutLength& GetMarginTop(void) const { return margin_top_; }
    public: const LayoutLength& GetMarginBottom(void) const { return margin_bottom_; }
    public: noz_float GetPaddingLeft(void) const { return padding_left_; }
    public: noz_float GetPaddingRight(void) const { return padding_right_; }
    public: noz_float GetPaddingTop(void) const { return padding_top_; }
    public: noz_float GetPaddingBottom(void) const { return padding_bottom_; }
    public: noz_float GetMinWidth (void) const {return min_width_;}
    public: noz_float GetMaxWidth (void) const {return max_width_;}
    public: noz_float GetMinHeight (void) const {return min_height_;}
    public: noz_float GetMaxHeight (void) const {return max_height_;}

    public: bool IsMaintainAspectRatio (void) const {return maintain_aspect_ratio_;}
    public: const Vector2& GetAspectRatio (void) const {return aspect_ratio_;}

    public: void SetWidth(const LayoutLength& width);
    public: void SetMinWidth(noz_float width);
    public: void SetMaxWidth(noz_float width);

    public: void SetHeight(const LayoutLength& height);
    public: void SetMinHeight(noz_float height);
    public: void SetMaxHeight(noz_float height);

    public: void SetMargin (const LayoutLength& margin);
    public: void SetMarginLeft(const LayoutLength& margin);
    public: void SetMarginRight(const LayoutLength& margin);
    public: void SetMarginTop(const LayoutLength& margin);
    public: void SetMarginBottom(const LayoutLength& margin);

    public: void SetPadding (noz_float padding);
    public: void SetPaddingLeft (noz_float padding);
    public: void SetPaddingTop (noz_float padding);
    public: void SetPaddingRight (noz_float padding);
    public: void SetPaddingBottom (noz_float padding);

    public: void SetMaintainAspectRatio (bool v);
    public: void SetAspectRatio (const Vector2& v);

    public: virtual Vector2 AdjustAvailableSize (const Vector2& available_size) override;

    public: virtual Vector2 AdjustMeasuredSize (const Vector2& measured_size) override;

    public: virtual Rect AdjustArrangeRect(const Rect& r) const override;

    public: virtual Rect Update (const Rect& arrange_rect, const Vector2& measured_size) override;

    public: virtual bool IsDependentOnMeasure (void) const override;
  };

} // namespace noz


#endif // __noz_LayoutTransform_h__



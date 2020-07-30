///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ScrollBar_h__
#define __noz_ScrollBar_h__

#include "ThumbTrack.h"

namespace noz {

  NOZ_ENUM() enum class ScrollBarVisibility {
    Auto,
    Visible
  };

  class RepeatButton;
  class Thumb;

  class ScrollBar : public Control {
    NOZ_OBJECT(DefaultStyle="{9F22093B-4C77-4FE6-8C2C-B71CBDEB1074}")

    public: ScrollEventHandler Scroll;

    NOZ_CONTROL_PART(Name=LineIncrementButton)
    private: ObjectPtr<RepeatButton> line_inc_button_;

    NOZ_CONTROL_PART(Name=LineDecrementButton)
    private: ObjectPtr<RepeatButton> line_dec_button_;

    NOZ_CONTROL_PART(Name=ThumbTrack)
    private: ObjectPtr<ThumbTrack> thumb_track_;

    NOZ_PROPERTY(Name=Maximum,Set=SetMaximum)
    private: noz_float maximum_;

    NOZ_PROPERTY(Name=Minimum,Set=SetMinimum)
    private: noz_float minimum_;

    NOZ_PROPERTY(Name=Value,Set=SetValue)
    private: noz_float value_;
    
    NOZ_PROPERTY(Name=ViewportSize,Set=SetViewportSize)
    private: noz_float viewport_size_;
    
    NOZ_PROPERTY(Name=LargeChange)
    private: noz_float large_change_;

    NOZ_PROPERTY(Name=SmallChange)
    private: noz_float small_change_;

    /// Default ScrollBar constructor
    public: ScrollBar (void);

    public: ~ScrollBar (void);

    public: noz_float GetMaximum(void) const {return maximum_;}
    public: noz_float GetMinimum(void) const {return minimum_;}
    public: noz_float GetValue(void) const {return value_;}
    public: noz_float GetViewportSize(void) const {return viewport_size_;}

    public: void SetMaximum (noz_float value);
    public: void SetMinimum (noz_float value);
    public: void SetValue (noz_float value);
    public: void SetViewportSize (noz_float value);
    public: void SetRange (noz_float minimum, noz_float maximum);

    public: void SetSmallChange (noz_float change);

    public: void SetLargeChange (noz_float change);

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnThumbTrackValueChanged (UINode* sender);
    private: void OnLineIncButton(UINode* sender);
    private: void OnLineDecButton(UINode* sender);
  };

} // namespace noz


#endif //__noz_ScrollBar_h__


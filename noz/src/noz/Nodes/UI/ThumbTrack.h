///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ThumbTrack_h__
#define __noz_ThumbTrack_h__

namespace noz {

  class Thumb;
  class RepeatButton;

  class ThumbTrack : public UINode {
    NOZ_OBJECT()

    public: ValueChangedEventHandler ValueChanged;

    NOZ_PROPERTY(Name=Maximum,Set=SetMaximum)
    private: noz_float maximum_;

    NOZ_PROPERTY(Name=Minimum,Set=SetMinimum)
    private: noz_float minimum_;

    NOZ_PROPERTY(Name=Value,Set=SetValue)
    private: noz_float value_;

    NOZ_PROPERTY(Name=ViewportSize,Set=SetViewportSize)
    private: noz_float viewport_size_;

    NOZ_PROPERTY(Name=LargeChange,Set=SetLargeChange)
    private: noz_float large_change_;

    NOZ_PROPERTY(Name=SmallChange,Set=SetSmallChange)
    private: noz_float small_change_;

    NOZ_PROPERTY(Name=Orientation,Set=SetOrientation)
    private: Orientation orientation_;

    NOZ_PROPERTY(Name=DecreaseButton,Set=SetDecreaseButton)
    private: ObjectPtr<RepeatButton> decrease_button_;

    NOZ_PROPERTY(Name=IncreaseButton,Set=SetIncreaseButton)
    private: ObjectPtr<RepeatButton> increase_button_;

    private: ObjectPtr<Thumb> thumb_;

    public: ThumbTrack (void);

    public: ~ThumbTrack (void);

    /// Return the current value 
    public: noz_float GetValue(void) const {return value_;}

    /// Return the normalized value [0-1]
    public: noz_float GetNormalizedValue (void) const {return (value_-minimum_) / (maximum_-minimum_);}

    public: noz_float GetMaximum (void) const {return maximum_;}
    public: noz_float GetMinimum (void) const {return minimum_;}
    public: noz_float GetViewportSize (void) const {return viewport_size_;}

    public: void SetValue (noz_float value);
    public: void SetMinimum (noz_float value);
    public: void SetMaximum (noz_float value);
    public: void SetRange(noz_float minimum, noz_float maximum);
    public: void SetViewportSize (noz_float view_port);
    public: void SetSmallChange (noz_float change);
    public: void SetLargeChange (noz_float change);
    public: void SetThumb (Thumb* thumb);
    public: void SetDecreaseButton (RepeatButton* button);
    public: void SetIncreaseButton (RepeatButton* button);
    public: void SetOrientation (Orientation orientation);

    private: void OnThumbDragDelta (DragDeltaEventArgs* args);
    private: void OnThumbDragCompleted (UINode* sender);
    private: void OnDecreaseButton (UINode* sender);
    private: void OnIncreaseButton (UINode* sender);

    protected: virtual void OnChildAdded (Node* child) override;
    protected: virtual void OnChildRemoved (Node* child) override;
    protected: virtual Vector2 MeasureChildren (const Vector2& avail) override;
    protected: virtual void ArrangeChildren (const Rect& r) override;
  };

} // namespace noz


#endif // __noz_ThumbTrack_h__


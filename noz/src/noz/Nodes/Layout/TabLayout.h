///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_TabLayout_h__
#define __noz_TabLayout_h__

namespace noz {

  NOZ_ENUM() enum class TabOverflow {
    /// Wrap the tabs to multiple lines. 
    Wrap,

    /// Shrink the tab sizes evenly below measured size to ensure they all fit
    Shrink,

    /// Clip all tabs that dont fit (tabs will automatically be collapsed)
    Clip,

    /// Allow the tabs to overflow
    Overflow
  };

  class TabLayout : public Layout {
    NOZ_OBJECT()

    private: struct Track {
      noz_float size_;
      noz_uint32 first_child_;
      noz_uint32 last_child_;
    };

    /// Orientation to grow in before wrapping
    NOZ_PROPERTY(Name=Orientation)
    private: Orientation orientation_;

    /// Overflow handling on tabs
    NOZ_PROPERTY(Name=TabOverflow)
    private: TabOverflow overflow_;

    private: std::vector<Track> tracks_;

    public: TabLayout(void);

    public: void SetOrientation(Orientation orientation);

    public: virtual Vector2 MeasureChildren (const Vector2& available_size) override;
    
    public: virtual void ArrangeChildren (const Rect& r) override;

    private: void WrapArrange (const Rect& r);
    private: void ClipArrange (const Rect& r);
    private: void OverflowArrange (const Rect& r);
    private: void ShrinkArrange (const Rect& r);

    private: noz_float ArrangeTrack (const Track& track, const Vector2& offset, noz_float u_max);

    public: virtual bool IsArrangeDependentOnChildren (void) const override {return true;}
  };

} // namespace noz


#endif //__noz_TabLayout_h__



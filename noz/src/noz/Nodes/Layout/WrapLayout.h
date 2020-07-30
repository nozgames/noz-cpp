///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_WrapLayout_h__
#define __noz_WrapLayout_h__

namespace noz {

  class WrapLayout : public Layout {
    NOZ_OBJECT()

    /// Orientation to grow in before wrapping
    NOZ_PROPERTY(Name=Orientation)
    private: Orientation orientation_;

    public: WrapLayout(void);

    public: void SetOrientation(Orientation orientation);

    public: virtual Vector2 MeasureChildren (const Vector2& available_size) override;
    
    public: virtual void ArrangeChildren (const Rect& r) override;

    public: virtual bool IsArrangeDependentOnChildren (void) const override {return true;}
  };

} // namespace noz


#endif //__noz_WrapLayout_h__



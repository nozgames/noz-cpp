///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_StackLayout_h__
#define __noz_StackLayout_h__

namespace noz {

  class StackLayout : public Layout {
    NOZ_OBJECT(EditorIcon="{1FA844FB-5F75-45E0-B459-53BCEC890BD6}")

    NOZ_PROPERTY(Name=Orientation)
    private: Orientation orientation_;

    /// Amount of spacing between nodes
    NOZ_PROPERTY(Name=Spacing,Set=SetSpacing)
    private: noz_float spacing_;

    public: StackLayout(void);

    public: void SetSpacing (noz_float spacing);

    public: void SetOrientation(Orientation orientation);

    public: virtual Vector2 MeasureChildren (const Vector2& a) override;
    
    public: virtual void ArrangeChildren (const Rect& r) override;

    public: virtual bool IsArrangeDependentOnChildren (void) const override {return true;}
  };

} // namespace noz


#endif //__noz_StackLayout_h__



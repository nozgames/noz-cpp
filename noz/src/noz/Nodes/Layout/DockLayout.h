///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DockLayout_h__
#define __noz_DockLayout_h__

namespace noz {

  NOZ_ENUM() enum class Dock {
    Left,
    Right,
    Top,
    Bottom
  };

  class DockLayout : public Layout {
    NOZ_OBJECT(EditorIcon="{B67DB665-65C5-4795-90E5-7CB01247A059}")

    NOZ_PROPERTY(Name=Dock)
    private: Dock dock_;

    NOZ_PROPERTY(Name=Spacing,Set=SetSpacing)
    private: noz_float spacing_;

    NOZ_PROPERTY(Name=LastChildFill,SetLastChildFill)
    private: bool last_child_fill_;

    public: DockLayout(void);

    public: void SetDock (Dock dock);

    public: void SetSpacing (noz_float spacing);

    public: void SetLastChildFill (bool v);

    public: virtual Vector2 MeasureChildren (const Vector2& available_size) override;
    
    public: virtual void ArrangeChildren (const Rect& r) override;

    public: virtual bool IsArrangeDependentOnChildren (void) const override {return true;}
  };

} // namespace noz


#endif //__noz_DockLayout_h__



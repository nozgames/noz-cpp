///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Popup_h__
#define __noz_Popup_h__

namespace noz {

  NOZ_ENUM() enum class PopupPlacement {
    Bottom,
    Top,
    Left,
    Right,
    Relative
  };

  class Popup : public UINode {
    NOZ_OBJECT()

    friend class Window;
 
    NOZ_PROPERTY(Name=Placement,Set=SetPlacement)
    private: PopupPlacement placement_;

    NOZ_PROPERTY(Name=PlacementTarget,Set=SetPlacementTarget)
    private: ObjectPtr<Node> placement_target_;

    NOZ_PROPERTY(Name=PlacementOffset,Set=SetPlacementOffset)
    private: Vector2 placement_offset_;

    private: ObjectPtr<Node> content_;

    private: ObjectPtr<Window> window_;

    public: bool IsOpen(void) const {return window_ != nullptr;}

    public: Popup (void);

    public: ~Popup (void);

    public: void Open (void);

    public: void Close (void);

    public: void SetPlacement (PopupPlacement placement);

    public: void SetPlacementTarget (Node* target);

    public: void SetPlacementOffset (const Vector2& offset);

    public: virtual void Arrange (const Rect& r) override;

    protected: virtual void OnChildAdded (Node* child) override;
  };

} // namespace noz


#endif //__noz_Popup_h__




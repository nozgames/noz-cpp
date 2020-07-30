///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ScrollView_h__
#define __noz_ScrollView_h__

#include "ScrollBar.h"

namespace noz {

  NOZ_ENUM() enum class ScrollMovementType {
    None,
    Elastic,
    Unrestricted,
    Clamped
  };

  class ScrollView : public UINode {
    NOZ_OBJECT()
   
    NOZ_PROPERTY(Name=HorizontalScrollBar,Type=ObjectPtr<ScrollBar>,Set=SetHorizontalScrollBar,Get=GetHorizontalScrollBar)
    NOZ_PROPERTY(Name=VerticalScrollBar,Type=ObjectPtr<ScrollBar>,Set=SetVerticalScrollBar,Get=GetVerticalScrollBar)

    NOZ_PROPERTY(Name=Horizontal,Type=bool,Get=GetHorizontal,Set=SetHorizontal)
    NOZ_PROPERTY(Name=Vertical,Type=bool,Get=GetVertical,Set=SetVertical)

    NOZ_PROPERTY(Name=HorizontalScrollBarVisibility,Type=ScrollBarVisibility,Get=GetHorizontalScrollBarVisibility,Set=SetHorizontalScrollBarVisibility)
    NOZ_PROPERTY(Name=VerticalScrollBarVisibility,Type=ScrollBarVisibility,Get=GetVerticalScrollBarVisibility,Set=SetVerticalScrollBarVisibility)

    NOZ_PROPERTY(Name=HorizontalOffset,Type=noz_float,Get=GetHorizontalOffset,Set=SetHorizontalOffset)
    NOZ_PROPERTY(Name=VerticalOffset,Type=noz_float,Get=GetVerticalOffset,Set=SetVerticalOffset)

    NOZ_PROPERTY(Name=MovementType)
    private: ScrollMovementType movement_type_;


    private: bool drag_;

    private: Vector2 drag_start_;
    
    /// Private content node used to arrange all logical children
    private: ObjectPtr<Node> content_;

    /// Scroll offset of the content
    private: Vector2 offset_;

    /// True if scrolling on the given axis is enabled 
    private: bool scroll_enabled_[2];

    /// Optional scrollbars per axis
    private: ObjectPtr<ScrollBar> scrollbar_[2];

    /// Scrollbar visibility
    private: ScrollBarVisibility scrollbar_visibility_[2];

    /// Default ScrollView constructor
    public: ScrollView (void);

    public: ~ScrollView (void);

    public: void SetScrollMovementType (ScrollMovementType smt);

    public: void ScrollToTop (void);

    public: void ScrollToBottom (void);

    public: void ScrollToLeft (void);

    public: void ScrollToRight (void);

    public: void ScrollToHorizontalOffset(noz_float offset);

    public: void SetHorizontal (bool v);

    public: void SetVertical (bool v);

    /// Sets the scrollbar to use for horizontal scrolling.  
    public: void SetHorizontalScrollBar (ScrollBar* scrollbar);

    /// Sets the scrollbar to use for vertical scrolling.  
    public: void SetVerticalScrollBar (ScrollBar* scrollbar);

    public: void SetHorizontalScrollBarVisibility (ScrollBarVisibility vis);

    public: void SetVerticalScrollBarVisibility (ScrollBarVisibility vis);

    public: void SetHorizontalOffset (noz_float offset);

    public: void SetVerticalOffset (noz_float offset);

    public: void BringIntoView (Node* node);

    public: void SetOffset (const Vector2& offset);

    public: ScrollBar* GetHorizontalScrollBar (void) const {return scrollbar_[Axis::Horizontal];}

    public: ScrollBar* GetVerticalScrollBar (void) const {return scrollbar_[Axis::Vertical];}

    public: noz_float GetHorizontalOffset (void) const {return offset_[Axis::Horizontal];}

    public: noz_float GetVerticalOffset (void) const {return offset_[Axis::Vertical];}

    public: ScrollBarVisibility GetHorizontalScrollBarVisibility (void) const {return scrollbar_visibility_[Axis::Horizontal];}

    public: ScrollBarVisibility GetVerticalScrollBarVisibility (void) const {return scrollbar_visibility_[Axis::Vertical];}

    public: Vector2 GetOffset (void) const {return offset_;}

    public: Vector2 GetViewportSize (void);

    public: Vector2 GetExtentSize (void) const;

    public: bool GetHorizontal (void) const {return scroll_enabled_[Axis::Horizontal];}

    public: bool GetVertical (void) const {return scroll_enabled_[Axis::Vertical];}

    public: Vector2 GetOffsetFromPosition (const Vector2& point);

    private: void OnScroll(UINode*);

    protected: virtual void ArrangeChildren (const Rect& r) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseOver (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;
    protected: virtual bool IsArrangeDependentOnChildren (void) const override {return true;}

    protected: virtual void OnChildAdded (Node* child) override;
  };

} // namespace noz


#endif //__noz_ScrollView_h__




///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Viewport_h__
#define __noz_Viewport_h__

namespace noz {

  class LayoutTransform;

  class Viewport : public Node {
    NOZ_OBJECT(EditorIcon="{9DEFC7E4-E929-4F9A-BBCA-84A72AED07D5}")

    friend class Node;

    private: NOZ_PROPERTY(Name=VirtualSize,Set=SetVirtualSize) noz_float virtual_size_;
    private: NOZ_PROPERTY(Name=VirtualOrientation,Set=SetVirtualOrientation) Orientation virtual_orientation_;

    protected: Matrix3 viewport_to_window_;

    protected: Matrix3 window_to_viewport_;

    protected: noz_float virtual_scale_;

    /// Scene the viewport belongs to
    protected: Scene* scene_;

    /// Cached parent viewport
    protected: Viewport* parent_viewport_;

    protected: Node* camera_container_;

    protected: LayoutTransform* camera_transform_;

    public: Viewport(void);

    public: ~Viewport(void);

    public: void SetVirtualSize (noz_float size);

    public: void SetVirtualOrientation(Orientation orientation);

    protected: virtual void Arrange (const Rect& r) override;
    protected: virtual void RenderOverride (RenderContext* rc) override;
    protected: virtual void OnLineageChanged (void) override;
    protected: virtual void ArrangeChildren(const Rect& ar) override;
    protected: virtual Vector2 MeasureChildren(const Vector2& a) override;
    protected: virtual void OnChildAdded (Node* child) override;
  };

} // namespace noz


#endif // __noz_Viewport_h__


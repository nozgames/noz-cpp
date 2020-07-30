///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RenderNode_h__
#define __noz_RenderNode_h__

namespace noz {

  class RenderNode : public Node {
    NOZ_OBJECT()

    /// True if the render mesh within the renderer is invalid
    protected: bool mesh_invalid_;

    public: RenderNode(void);

    /// Invalidate the render state.
    public: void Invalidate (void);

    protected: virtual bool DrawMesh (RenderContext* rc) = 0;

    protected: virtual Vector2 MeasureMesh (const Vector2& a) {return Vector2::Zero;}

    protected: virtual void UpdateMesh (const Rect& r) = 0;

    protected: virtual void Arrange (const Rect& r) override;
    protected: virtual Vector2 MeasureChildren (const Vector2& a) override;
    protected: virtual bool Render(RenderContext* rc, Rect& render_rect) override;
  };

} // namespace noz


#endif // __noz_RenderNode_h__


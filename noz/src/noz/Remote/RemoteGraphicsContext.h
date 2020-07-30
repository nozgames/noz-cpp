///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RemoteGraphicsContext_h__
#define __noz_RemoteGraphicsContext_h__

#include <noz/Platform/RenderContextHandle.h>

namespace noz {

  class RemoteFrame;

  class RemoteGraphicsContext : public noz::Platform::RenderContextHandle {    
    private: RemoteFrame* frame_;

    public: RemoteGraphicsContext(RemoteFrame* frame);

    /// Begin a frame of rendering.
    public: virtual void Begin (const Vector2& size, RenderTarget* target) { }

    /// End a frame of rendering.
    public: virtual void End (void) { }

    /// Push a clipping rectangle with optional mask onto the clipping stack
    public: virtual bool PushMask (const Rect& clip, Image* mask, Rect& render_rect) { return false; }

    /// Pop the last pushed clipping rectangle off of the clipping stack.
    public: virtual void PopMask  (void) {}

    /// Set the current transform
    public: virtual void SetTransform (const Matrix3& transform);

    /// Draw a debug line
    public: virtual void DrawDebugLine (const Vector2& v1, const Vector2& v2, Color color) { }
    
    /// Render the given mesh
    public: virtual bool Draw (RenderMesh* mesh, noz_float opacity) override;
  };        

} // namespace noz


#endif //__noz_RemoteGraphicsContext_h__



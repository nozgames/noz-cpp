///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RenderContext_h__
#define __noz_RenderContext_h__

#include "Color.h"

namespace noz {

  class Font;
  class Image;
  class Sprite;
  class RenderMesh;
  class RenderTarget;

  namespace Platform { class RenderContextHandle; }

  class RenderContext : public Object {
    NOZ_OBJECT(Abstract)

    protected: struct State {
      /// Number of masks pushed 
      noz_uint32 mask_count_;

      /// Number of matricies pushed
      noz_uint32 matrix_count_;
    };

    /// Stack of matricies
    protected: std::vector<Matrix3> matrix_stack_;

    /// State stack
    protected: std::vector<State> states_;

    /// Stack of opacities
    protected: std::vector<noz_float> opacity_stack_;

    /// Handle to platform implementation of graphics context
    protected: Platform::RenderContextHandle* handle_;

    protected: bool own_handle_;

    public: RenderContext (void);

    /// Create a graphics context with a specific handle
    public: RenderContext (Platform::RenderContextHandle* handle);

    /// Default destructor
    public: ~RenderContext(void);

    /// Begin rendering a frame.
    public: void Begin (const Vector2& size, RenderTarget* target);

    /// End rendering a frame.
    public: void End (void);

    /// Draw the given render mesh
    public: bool Draw (RenderMesh* mesh);

    /// Push a transform to the transform stack
    public: void PushMatrix (void);

    /// Pop the top matrix from the stack
    public: void PopMatrix (void);

    /// Multiply the matrix at the top of the transform stack by the given matrix
    public: void MultiplyMatrix (const Matrix3& mat);

    /// Set the matrix at the top of the matrix stack with the given matrix
    public: void SetMatrix(const Matrix3& mat);

    public: void PushState (void);

    public: void PopState (void);

    /// Push a mask to the stack
    public: bool PushMask (const Rect& r, Image* mask, Rect& render_rect);

    /// Pop a previously pushed mask from the stack
    public: void PopMask (void);

    public: void PushOpacity (noz_float opacity);

    public: void PopOpacity (void);

    public: void DrawDebugLine(const Vector2& v1, const Vector2& v2, Color color);

    public: Platform::RenderContextHandle* GetHandle(void) {return handle_;}
  };        

} // namespace noz

#include "Image.h"
#include "Font.h"
#include "Sprite.h"
#include "RenderMesh.h"
#include "RenderTarget.h"

#endif // __noz_Graphics_RenderContext_h__


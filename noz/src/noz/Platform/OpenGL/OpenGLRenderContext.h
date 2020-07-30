///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLRenderContext_h__
#define __noz_Platform_OpenGLRenderContext_h__

#include "../RenderContextHandle.h"
#include "OpenGLBatch.h"

namespace noz {
namespace Platform {

  class OpenGLColorProgram;
  class OpenGLTextureProgram;
  class OpenGLTextureA8Program;
  class OpenGLTextureSDFProgram;

  class OpenGLRenderContext : public RenderContextHandle {
    /// Size of view.
    protected: Vector2 view_size_;

    protected: RenderTarget* target_;

    /// Current transform
    protected: Matrix3 transform_;

    /// Projection transform
    protected: Matrix4 projection_;

    protected: LinkedList<OpenGLBatch> batches_;

    protected: LinkedList<OpenGLBatch> free_batches_;

    protected: OpenGLState current_state_;

    protected: OpenGLColorProgram* program_color_;

    protected: OpenGLTextureProgram* program_texture_;

    protected: OpenGLTextureA8Program* program_texture_a8_;

    protected: OpenGLTextureSDFProgram* program_texture_sdf_;

    protected: GLuint vertex_buffer_id_;

    protected: std::vector<noz_byte> vertex_buffer_;

    protected: GLuint index_buffer_id_;

    protected: std::vector<noz_uint16> index_buffer_;

    protected: noz_byte stencil_depth_;

    protected: noz_uint32 stencil_id_;

    public: OpenGLRenderContext (void);

    public: ~OpenGLRenderContext (void);

    public: virtual void Begin(const Vector2& size, RenderTarget* target) override;
    public: virtual void End (void) override;

    /// Draw the given render mesh
    public: virtual bool Draw (RenderMesh* mesh, noz_float opacity) override;

    /// Set the current model view t ransform
    public: virtual void SetTransform (const Matrix3& transform) override;

    /// Push a clipping rectangle with optional mask onto the clipping stack
    public: virtual bool PushMask (const Rect& clip, Image* mask, Rect& render_rect) override;

    /// Pop the last pushed clipping rectangle off of the clipping stack.
    public: virtual void PopMask (void) override;


    public: void DrawDebugLine(const Vector2& v1, const Vector2& v2, Color color);
    public: void DrawDebugRectangle (const Rect& r, Color color);

    private: void DrawDebug (void);

    private: void PresentBatches (void);

    private: static OpenGLTexture* GetTexture(Image* image);

    private: Vector2 PixelCorrect(const Vector2& v) {
      return Vector2(
        ((noz_float)(int)(v.x)) + 0.375f,
        ((noz_float)(int)(v.y)) + 0.375f
      );
    }

    private: OpenGLBatch* GetBatch (const OpenGLState& state, const Rect& rect, noz_uint32 vc, noz_uint32 ic);

    private: void SetState (const OpenGLState& state);

    private: OpenGLProgram* GetProgramForImage (Image* image);
    private: noz_uint32 GetFlagsForImage (Image* image);
  };        


} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLRenderContext_h__


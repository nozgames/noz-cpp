///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLRenderTarget_h__
#define __noz_Platform_OpenGLRenderTarget_h__

#include "../RenderTargetHandle.h"

namespace noz {
namespace Platform {

  class OpenGLRenderTarget : public RenderTargetHandle {
    protected: GLuint frame_buffer_id_;
    protected: GLuint render_buffer_id_;
    protected: RenderTarget* target_;

    // Default constructor
    public: OpenGLRenderTarget(RenderTarget* target);

    // Default destructor
    public: ~OpenGLRenderTarget(void);

    public: void Bind (void);

    public: void UnBind (void);
  };
    
} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLRenderTarget_h__

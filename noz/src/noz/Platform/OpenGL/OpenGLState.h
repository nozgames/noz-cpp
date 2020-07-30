///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLState_h__
#define __noz_Platform_OpenGLState_h__

#include "OpenGLVertex.h"

namespace noz {
namespace Platform {

  class OpenGLProgram;
  class OpenGLTexture;

  struct OpenGLState {
    /// Indicates scene node requires alpha blending.
    static const noz_uint32 FlagAlpha                 = NOZ_BIT(0);

    /// Indicates that the scene node should use a linear filter to render its image.
    static const noz_uint32 FlagLinearFilterOverride  = NOZ_BIT(1);

    /// True if statistics should be recorded
    static const noz_uint32 FlagStatistics    = NOZ_BIT(2);

    /// True if the batch is a debug state
    static const noz_uint32 FlagDebug         = NOZ_BIT(3);

    /// True if rendering should occur in the stencil buffer
    static const noz_uint32 FlagStencil       = NOZ_BIT(4);

    /// Current vertex format.
    OpenGLVertexFormat vertex_format_;

    /// Flags 
    noz_uint32 flags_;

    /// GLSL Program to use for batch.
    OpenGLProgram* program_;

    /// Textures bound to batch
    OpenGLTexture* texture_[2];

    /// GL primitive type
    GLenum primitive_type_;

    /// The current depth of the stencil test (represents the nesting level, 0-8)
    noz_byte stencil_depth_;

    noz_uint32 stencil_id_;

    bool operator == (const OpenGLState& state) const {
      if(flags_ != state.flags_) return false;
      if(program_ != state.program_) return false;
      if(vertex_format_ != state.vertex_format_) return false;
      if(texture_[0] != state.texture_[0]) return false;
      if(texture_[1] != state.texture_[1]) return false;
      if(primitive_type_ != state.primitive_type_) return false;
      if(stencil_depth_ != state.stencil_depth_) return false;
      if(stencil_id_ != state.stencil_id_) return false;
      return true;
    }

    bool operator!= (const OpenGLState& state) const {
      return !(*this == state);
    }
  };


} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLState_h__


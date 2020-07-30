///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLVertex_h__
#define __noz_Platform_OpenGLVertex_h__

namespace noz {
namespace Platform {

  enum class OpenGLVertexFormat {
    Unknown,
    XYZUVC,
    XYZC,
  };

  /// Vertex with X, Y, Z, U, V, and Color
  struct OpenGLVertexXYZUVC {
    Vector2 uv;
    noz_uint32 c;
    Vector3 xyz;
  };

  /// Vertex with X, Y, Z, and Color
  struct OpenGLVertexXYZC {
    noz_uint32 c;
    Vector3 xyz;
  };


} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLVertex_h__


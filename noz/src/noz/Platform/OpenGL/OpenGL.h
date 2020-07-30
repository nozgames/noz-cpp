///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGL_h__
#define __noz_Platform_OpenGL_h__

using namespace noz;

namespace noz { namespace Platform { 
  noz_int32 GetOpenGLShaderVersion (void);
}}

#if defined(NOZ_WINDOWS)
#include <external/glew-1.10.0/include/GL/glew.h>
#include <external/glew-1.10.0/include/GL/wglew.h>
#elif defined(NOZ_OSX)
#include <GLUT/GLUT.h>
#elif defined(NOZ_IOS)
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#endif

#include "OpenGLTexture.h"
#include "OpenGLProgram.h"
#include "OpenGLRenderContext.h"


#endif // __noz_Platform_OpenGL_h__



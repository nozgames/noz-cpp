///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLTextureA8Program_h__
#define __noz_Platform_OpenGLTextureA8Program_h__

#include "OpenGLProgram_Texture.h"

namespace noz {  
namespace Platform {

  
  class OpenGLTextureA8Program : public OpenGLTextureProgram {
    public: OpenGLTextureA8Program (void) {
    }
  
    protected: virtual String source_fragment (void) const {
      if(GetOpenGLShaderVersion() == 100) {
        return String (
          #include "shaders/gles2/TextureA8.frag.h"
        );
      }

      return String (
        #include "shaders/gles3/TextureA8.frag.h"
      );
    }
  };  


} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLTextureA8Program_h__

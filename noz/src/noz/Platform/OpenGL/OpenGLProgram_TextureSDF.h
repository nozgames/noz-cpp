///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLTextureSDFProgram_h__
#define __noz_Platform_OpenGLTextureSDFProgram_h__

#include "OpenGLProgram_Texture.h"

namespace noz {  
namespace Platform {

  
  class OpenGLTextureSDFProgram : public OpenGLTextureProgram {
    public: OpenGLTextureSDFProgram (void) {
    }
  
    protected: virtual String source_fragment (void) const {
      if(GetOpenGLShaderVersion() == 100) {
        return String(
          #include "shaders/gles2/TextureSDF.frag.h"
        );
      }

      return String(
        #include "shaders/gles3/TextureSDF.frag.h"
      );
    }
  };  


} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLTextureSDFProgram_h__

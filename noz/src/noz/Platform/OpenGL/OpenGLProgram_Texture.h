///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLTextureProgram_h__
#define __noz_Platform_OpenGLTextureProgram_h__

#include "OpenGLProgram.h"

namespace noz {  
namespace Platform {
  

  class OpenGLTextureProgram : public OpenGLProgram {
    private: GLuint uniform_texture_;

    public: OpenGLTextureProgram(void) {
    }
  
    protected: virtual String source_vertex (void) const {
      if(GetOpenGLShaderVersion() == 100) {
        return String(
          #include "shaders/gles2/Texture.vert.h"
        );
      }

      return String(
        #include "shaders/gles3/Texture.vert.h"
      );
    }
    
    protected: virtual String source_fragment (void) const {
      if(GetOpenGLShaderVersion() == 100) {
        return String(
          #include "shaders/gles2/Texture.frag.h"
        );
      }

      return String(
        #include "shaders/gles3/Texture.frag.h"
      );
    }
  
    public: virtual void pre_link(GLuint i_program) {  
      glBindAttribLocation(i_program,0,"a_uv");
      glBindAttribLocation(i_program,1,"a_color");
      glBindAttribLocation(i_program,2,"a_position");
    }
    
    public: virtual void post_link(GLuint i_program) {
      uniform_texture_ = glGetUniformLocation(i_program, "u_texture");

      // Use texture0 for the texture.
      glUniform1i(uniform_texture_,0);
    }    
  };  
  

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLTextureProgram_h__

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGL_OpenGLGlyphSDFProgram_h__
#define __noz_Platform_OpenGL_OpenGLGlyphSDFProgram_h__

#include "OpenGLProgram.h"

namespace noz {  
namespace Platform {
namespace OpenGL {
  
  class OpenGLGlyphSDFProgram : public OpenGLProgram {
    private: GLuint uniform_texture_;

    public: OpenGLGlyphSDFProgram(void) {
    }
  
    protected: virtual String source_vertex (void) const {
      return String(
        #include "shaders/GlyphSDF.vert.h"
      );
    }
    
    protected: virtual String source_fragment (void) const {
      return String(
        #include "shaders/GlyphSDF.frag.h"
      );
    }
  

    public: virtual void pre_link(GLuint i_program) {  
      glBindAttribLocation(i_program,0,"a_position");
      glBindAttribLocation(i_program,1,"a_uv");
      glBindAttribLocation(i_program,2,"a_color");
    }
    
    public: virtual void post_link(GLuint i_program) {
      uniform_texture_ = glGetUniformLocation(i_program, "u_texture");

      // Use texture0 for the texture.
      glUniform1i(uniform_texture_,0);      
    }    
  };  
  

} // namespace OpenGL
} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGL_OpenGLGlyphSDFProgram_h__

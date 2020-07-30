///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGL_OpenGLGlyphProgram_h__
#define __noz_Platform_OpenGL_OpenGLGlyphProgram_h__

#include "OpenGLProgram.h"

namespace noz {  
namespace Platform {
namespace OpenGL {
  
  class OpenGLGlyphProgram : public OpenGLProgram {
    private: GLuint uniform_texture_;
    private: GLuint uniform_pixel_;

    public: OpenGLGlyphProgram(void) {
    }
  
    protected: virtual String source_vertex (void) const {
      return String(
        #include "shaders/Glyph.vert.h"
      );
    }
    
    protected: virtual String source_fragment (void) const {
      return String(
        #include "shaders/Glyph.frag.h"
      );
    }
  

    public: virtual void pre_link(GLuint i_program) {  
      glBindAttribLocation(i_program,0,"in_position");
      glBindAttribLocation(i_program,1,"in_uv");
      glBindAttribLocation(i_program,2,"in_color");
      glBindAttribLocation(i_program,3,"in_st");
    }
    
    public: virtual void post_link(GLuint i_program) {
      uniform_texture_ = glGetUniformLocation(i_program, "texture");
      uniform_pixel_ = glGetUniformLocation(i_program, "pixel");

      glUniform3f( uniform_pixel_, 1.0/512, 1.0/512, 3.0f);

      // Use texture0 for the texture.
      glUniform1i(uniform_texture_,0);      
    }    
  };  
  

} // namespace OpenGL
} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGL_OpenGLGlyphProgram_h__

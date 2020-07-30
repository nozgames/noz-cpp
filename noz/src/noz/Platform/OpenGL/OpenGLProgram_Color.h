///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLColorProgram_h__
#define __noz_Platform_OpenGLColorProgram_h__

#include "OpenGLProgram.h"

namespace noz {  
namespace Platform {

  class OpenGLColorProgram : public OpenGLProgram {
    public: OpenGLColorProgram(void) {
    }

    protected: virtual String source_vertex(void) const override {
      if(GetOpenGLShaderVersion()==100) {
        return String(
          #include "shaders/gles2/Color.vert.h"
        );
      }

      return String(
        #include "shaders/gles3/Color.vert.h"
      );
    }
    
    protected: virtual String source_fragment(void) const override {
      if(GetOpenGLShaderVersion()==100) {
        return String(
          #include "shaders/gles2/Color.frag.h"
        );
      }

      return String(
        #include "shaders/gles3/Color.frag.h"
      );
    }
    
    protected: virtual void pre_link(GLuint i_program) {
      glBindAttribLocation(i_program,0,"a_color");
      glBindAttribLocation(i_program,1,"a_position");
    }

    protected: virtual void post_link(GLuint i_program) {
    }  
  };
  

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLColorProgram_h__

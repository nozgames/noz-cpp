///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLProgram_h__
#define __noz_Platform_OpenGLProgram_h__

#define STRINGIFY(x) #x

namespace noz {
namespace Platform {

  class OpenGLProgram {
    private: GLuint id_;
    private: GLint  uniform_modelview_;
    private: GLint  uniform_projection_;
  
    public: OpenGLProgram(void) {
      id_=0;
    }
  
    public: void use(void) {
      glUseProgram(id_);
    }    

    public: void set_projection(const Matrix4& mat) {
      glUniformMatrix4fv(uniform_projection_, 1, 0, mat.d);
    }
    
    public: bool compile (void) {
      GLuint vs=compile_shader(source_vertex(),GL_VERTEX_SHADER);
      GLuint fs=compile_shader(source_fragment(),GL_FRAGMENT_SHADER);

      // Create the program and attach the vertex and fragment shaders to it
      id_ = glCreateProgram();
      glAttachShader(id_, vs);
      glAttachShader(id_, fs);
      
      pre_link(id_);
      
      // Link the program    
      glLinkProgram(id_);

      GLint linkSuccess;
      glGetProgramiv(id_, GL_LINK_STATUS, &linkSuccess);
      if (linkSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetProgramInfoLog(id_, sizeof(messages), 0, &messages[0]);
        Console::WriteLine(messages);
        return false;
      }
     
      glUseProgram(id_);

      // Store the uniform locations for the modelview and projection matricies      
      uniform_projection_ = glGetUniformLocation(id_,"u_projection");
      
      post_link(id_);           
      
      return true;
    }

    /**
     * Called internally to compile the given shader source
     */
    protected: GLuint compile_shader(String src, GLenum type) {
      StringBuilder sb;
#if defined(NOZ_IOS)
      if(GetOpenGLShaderVersion()!=100) {
        sb.Append("#version 300 es\r\n");
      }
#else
      sb.Append("#version 150 core\r\n");
#endif
      sb.Append(src);
      src = sb.ToString();
    
      GLuint sh=glCreateShader(type);
      int shlen=src.GetLength();
      const char* cstr = src.ToCString();
      glShaderSource(sh,1,&cstr,&shlen);
      glCompileShader(sh);

      GLchar messages[256];
      GLint compileStatus;
      glGetShaderiv(sh, GL_COMPILE_STATUS, &compileStatus);
      if (compileStatus == GL_FALSE) {
        glGetShaderInfoLog(sh, sizeof(messages), 0, &messages[0]);
        Console::WriteLine(messages);
        return 0;
      }
      return sh;
    }
    
    /**
     * Called internally by the program to retrieve the vertex shader source
     */
    protected: virtual String source_vertex(void) const = 0;

    /**
     * Called internally by the program to retrieve the fragment shader source
     */
    protected: virtual String source_fragment(void) const = 0;

    /**
     * Called internally prior to linking the program
     */
    protected: virtual void pre_link(GLuint i_program) {}

    /**
     * Called internally after linking the program
     */
    protected: virtual void post_link(GLuint i_program) {}    
  };
    

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLProgram_h__


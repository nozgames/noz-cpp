///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "OpenGL.h"

using namespace noz;
using namespace noz::Platform;

static GLenum gl_format[noz_int32(ImageFormat::Count)] = {
  GL_RGB,
  GL_RGBA,
  GL_ALPHA,
  GL_ALPHA
};    

static GLenum gl_type[noz_int32(ImageFormat::Count)] = {
  GL_UNSIGNED_BYTE,
  GL_UNSIGNED_BYTE,
  GL_UNSIGNED_BYTE,
  GL_UNSIGNED_BYTE,
};     

static GLenum gl_depth[noz_int32(ImageFormat::Count)] = {
  3,
  4,
  1,
  1,
};    

ImageHandle* ImageHandle::CreateInstance(Image* image) {
  return new OpenGLTexture(image);
}

OpenGLTexture::OpenGLTexture(Image* image) {
  id_ = 0;
  image_ = image;
  state_dirty_ = true;
  dirty_ = true;
  current_filter_ = ImageFilterMode::None;
}

OpenGLTexture::~OpenGLTexture(void) {
}

void OpenGLTexture::UnBind (void) {
  glBindTexture(GL_TEXTURE_2D,0);
}

void OpenGLTexture::Bind(noz_int32 texture) {
  if(id_ == 0 && buffer_.empty()) {
    Lock();
    Unlock();
  }

  if(buffer_.size()) {
    noz_assert(!image_->IsLocked());

    // Create an new open gl texture and bind it
    if(id_==0) glGenTextures (1, &id_);
	
	  // If a proper id is generated..
	  if (id_==0) return;

	  glBindTexture (GL_TEXTURE_2D, id_);
  		
    noz_uint32 rw = Math::NextPow2(image_->GetWidth());
    noz_uint32 rh = Math::NextPow2(image_->GetHeight());
    noz_uint32 depth = gl_depth[noz_int32(image_->GetFormat())];

	  glTexImage2D ( 
	    GL_TEXTURE_2D, 
	    0,
		  gl_format[noz_int32(image_->GetFormat())],
		  (GLsizei)image_->GetWidth(),
		  (GLsizei)image_->GetHeight(),
      0,
		  gl_format[noz_int32(image_->GetFormat())],
		  gl_type[noz_int32(image_->GetFormat())],
      &buffer_[0]
    );

    // TODO: this should be part of scene node state

    buffer_.clear();
    buffer_.shrink_to_fit();    
  }

  if(id_!=0) {
    glActiveTexture(GL_TEXTURE0 + texture);
    glBindTexture(GL_TEXTURE_2D,id_);

    if(state_dirty_) {
      SetFilterMode(image_->GetFilterMode());

      if(image_->GetWrapMode() == ImageWrapMode::Repeat) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
      state_dirty_ = false;
    }
  }
}

void OpenGLTexture::SetFilterMode(ImageFilterMode filter) {
  current_filter_ = filter;
  if(current_filter_ == ImageFilterMode::Linear) {
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  } else {
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
  }
}

noz_byte* OpenGLTexture::Lock(void) {
  // If the buffer data is still available just return it
  if(buffer_.size()) {
    return &buffer_[0];
  }

  // Resize the buffer to hold the data
  buffer_.resize(image_->GetStride() * image_->GetHeight());

  // Retrieve the data from the texture
  if(id_ != 0) {
#if !defined(NOZ_IOS)
    glBindTexture(GL_TEXTURE_2D,id_);
    glGetTexImage(
      GL_TEXTURE_2D,
      0,
      gl_format[noz_int32(image_->GetFormat())],
      gl_type[noz_int32(image_->GetFormat())],
      &buffer_[0]
    );
    glBindTexture(GL_TEXTURE_2D,0);
#else
    noz_assert(false);
#endif
  }

  return &buffer_[0];
}

void OpenGLTexture::Unlock(void) {
}

void OpenGLTexture::InvalidateState (void) {
  state_dirty_ = true;
}

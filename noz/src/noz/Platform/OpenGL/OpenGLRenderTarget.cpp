///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Render/RenderTarget.h>
#include "OpenGL.h"
#include "OpenGLRenderTarget.h"

using namespace noz;
using namespace noz::Platform;


RenderTargetHandle* RenderTargetHandle::CreateInstance(RenderTarget* target) {
  return new OpenGLRenderTarget(target);
}

OpenGLRenderTarget::OpenGLRenderTarget(RenderTarget* target) {
  render_buffer_id_ = 0;
  frame_buffer_id_ = 0;
  target_ = target;
}

OpenGLRenderTarget::~OpenGLRenderTarget(void) {
}

void OpenGLRenderTarget::Bind (void) {
  if(target_->GetImage()==nullptr) return;

  if(render_buffer_id_ == 0) {
    OpenGLTexture* texture = ((OpenGLTexture*)target_->GetImage()->GetHandle());
    noz_assert(texture);

    glGenRenderbuffers(1, &render_buffer_id_);
    glGenFramebuffers(1, &frame_buffer_id_);

    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);

    texture->Bind();

    glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_id_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, target_->GetImage()->GetWidth(), target_->GetImage()->GetHeight());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->GetId(), 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer_id_);

    texture->UnBind();

    glBindRenderbuffer(GL_RENDERBUFFER, 0); 
  }

  glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_id_);
  glClearColor(1,0,0,1);
  glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLRenderTarget::UnBind(void) {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



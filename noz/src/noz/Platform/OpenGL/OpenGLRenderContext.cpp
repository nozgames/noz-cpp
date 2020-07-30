///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "OpenGL.h"
#include "OpenGLProgram_Color.h"
#include "OpenGLProgram_Texture.h"
#include "OpenGLProgram_TextureA8.h"
#include "OpenGLProgram_TextureSDF.h"
#include "OpenGLRenderTarget.h"

using namespace noz;
using namespace noz::Platform;

#define ENABLE_AUTO_BATCHING          1     /// If true draw commands will be batched
#define ENABLE_BUFFER_SHARE           1     /// If true the vertex buffer will be shared between batches
#define ENABLE_BUFFER_SHARE_FORMATS   1     /// If true and buffer share is enabled the buffer will be shared by different formats
#define ENABLE_REUSE_BATCHES          1     /// If true batches will be saved into a free list and reused rather than allocating new ones

static Int32ConsoleVariable* ogl_debug_batches = nullptr;
static Int32ConsoleVariable* ogl_debug_counters = nullptr;


RenderContextHandle* RenderContextHandle::CreateInstance(void) {
  return new OpenGLRenderContext;
}


OpenGLRenderContext::OpenGLRenderContext(void) {
  if(ogl_debug_batches==nullptr) ogl_debug_batches = Console::RegisterInt32Variable("ogl_debug_batches", 0, 0, 1);
  if(ogl_debug_counters==nullptr) ogl_debug_counters = Console::RegisterInt32Variable("ogl_debug_counters", 0, 0, 1);

  program_color_= nullptr;
  program_texture_ = nullptr;
  program_texture_a8_ = nullptr;
  program_texture_sdf_ = nullptr;
  transform_.identity();
  stencil_depth_ = 0;
  stencil_id_= 0;

  // Initialize default state.
  current_state_.flags_ = OpenGLState::FlagStatistics;
  current_state_.program_ = nullptr;
  current_state_.texture_[0] = nullptr;
  current_state_.texture_[1] = nullptr;
  current_state_.vertex_format_ = OpenGLVertexFormat::Unknown;
  current_state_.primitive_type_ = GL_TRIANGLES;

  // Initialize the vertex buffer.
  vertex_buffer_id_ = 0;
  vertex_buffer_.resize(1024 * sizeof(OpenGLVertexXYZUVC));

  // Initialize the index buffer
  index_buffer_id_ = 0;
  index_buffer_.resize(4096);
}

OpenGLRenderContext::~OpenGLRenderContext (void) {
  delete program_color_;
  delete program_texture_;
  delete program_texture_a8_;
  delete program_texture_sdf_;

  vertex_buffer_.clear();
  index_buffer_.clear();
}

void OpenGLRenderContext::Begin(const Vector2& size, RenderTarget* target) {
  view_size_ = size;
  target_= target;
  stencil_depth_  = 0;
  stencil_id_ = 0;

  if(target) {
    ((OpenGLRenderTarget*)target->GetHandle())->Bind();
    projection_.ortho(0.0f,size.x,0.0f,size.y,-1000.0f,1000.0f);  
  } else {
    projection_.ortho(0.0f,size.x,size.y,0.0f,-1000.0f,1000.0f);  
  }

  if(nullptr==program_texture_) {
    program_texture_=new OpenGLTextureProgram;
    program_texture_->compile();
  }

  if(nullptr==program_color_) {
    program_color_=new OpenGLColorProgram;
    program_color_->compile();
  }

  if(nullptr==program_texture_a8_) {
    program_texture_a8_=new OpenGLTextureA8Program;
    program_texture_a8_->compile();
  }

  if(nullptr==program_texture_sdf_) {
    program_texture_sdf_=new OpenGLTextureSDFProgram;
    program_texture_sdf_->compile();
  }

#if 0
  if(nullptr==program_text_with_brush_) {
    program_text_with_brush_=new TextureProgramTextWithBrush;
    program_text_with_brush_->compile();
  }
#endif

  program_color_->use();
  program_color_->set_projection(projection_);

  program_texture_->use();
  program_texture_->set_projection(projection_);

  program_texture_a8_->use();
  program_texture_a8_->set_projection(projection_);

  program_texture_sdf_->use();
  program_texture_sdf_->set_projection(projection_);

  //program_glyph_->use();
  //program_glyph_->set_projection(projection_);

#if 0
  program_text_with_brush_->use();
  program_text_with_brush_->set_projection(projection_);
#endif

  // TODO: dont set viewport every time.
  if(target) {
    glViewport(0,0,GLsizei(target->GetImage()->GetWidth()),GLsizei(target->GetImage()->GetHeight()));
  } else {
    glViewport(0,0,GLsizei(size.x),GLsizei(size.y));
  }

  glClear(GL_DEPTH_BUFFER_BIT);

  // TODO: dont set depth test every time
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_STENCIL_TEST);
}

void OpenGLRenderContext::End(void) {
  if(ogl_debug_batches->GetValue()) DrawDebug();

  // Present the batches.
  PresentBatches();

  if(target_) {
    ((OpenGLRenderTarget*)target_->GetHandle())->UnBind();
    target_ = nullptr;
  }
}

void OpenGLRenderContext::PresentBatches(void) {
  noz_uint32 counter_batches = 0;
  noz_uint32 counter_verticies = 0;
  noz_uint32 counter_indicies = 0;
  noz_uint32 counter_buffer_write = 0;
  noz_uint32 counter_buffer_write_total = 0;
  noz_uint32 counter_buffer_write_used = 0;

  // Ensure program is reset every frame
  current_state_.program_ = nullptr;
  current_state_.stencil_depth_ = 0;
  current_state_.stencil_id_ = 0;

  while(batches_.GetCount()>0) {
    noz_uint32 vbsize = 0;
    noz_uint32 vbcount = 0;
    noz_uint32 ibsize = 0;
    noz_uint32 bcount = 0;
    
    // Populate the vertex buffer with as much batch data as we can
    OpenGLBatch* last = nullptr;
    for(auto it=batches_.GetFirst(); it!=nullptr; last=it->GetValue(),it=it->GetNext()) {
      OpenGLBatch* b = it->GetValue();
      noz_assert(b);
      
#if ENABLE_BUFFER_SHARE && !ENABLE_BUFFER_SHARE_FORMATS
      if(last && last->state_.vertex_format_ != b->state_.vertex_format_) break;
#endif
  
      noz_uint32 bvsize = b->vertex_count_ * b->vertex_size_;

      // Ensure the batch vertex data will fit
      if(vbsize + bvsize > vertex_buffer_.size()) break;

      // Ensure the batch index data will fit
      if(ibsize + b->index_count_ > index_buffer_.size()) break;

      // Copy the batch vertex data
      memcpy(&vertex_buffer_[vbsize], &b->vertex_data_[0], bvsize );

      // Copy the batch index data.
      if(b->index_count_) {
        memcpy(&index_buffer_[ibsize], &b->index_data_[0], sizeof(noz_uint16) * b->index_count_ );

        b->index_offset_ = ibsize;
        ibsize += b->index_count_;
      }

      b->vertex_offset_ = vbsize;
      vbsize += bvsize;
      vbcount += b->vertex_count_;

      bcount++;

      if(!(b->state_.flags_ & OpenGLState::FlagDebug)) {
        counter_verticies += b->vertex_count_;
        counter_indicies += b->index_count_;
      }

#if !ENABLE_BUFFER_SHARE
      break;
#endif          
    }

    // Push the vertex buffer 
    if(vertex_buffer_id_ == 0) {
      glGenBuffers(1,&vertex_buffer_id_);
      glBindBuffer(GL_ARRAY_BUFFER,vertex_buffer_id_);

      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glEnableVertexAttribArray(2);
    }
    glBufferData(GL_ARRAY_BUFFER,vertex_buffer_.size(),&vertex_buffer_[0],GL_STREAM_DRAW);

    // Push the index buffer
    if(index_buffer_id_ == 0) {
      glGenBuffers(1,&index_buffer_id_);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,index_buffer_id_);
    }
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,index_buffer_.size()*sizeof(noz_uint16),&index_buffer_[0],GL_STREAM_DRAW);

    counter_buffer_write++;
    counter_buffer_write_total += vertex_buffer_.size();
    counter_buffer_write_used += vbsize;

    for(;bcount>0;bcount--) {
      // Pop top batch.
      OpenGLBatch* b = batches_.GetFirst()->GetValue();
      batches_.RemoveFirst();
      noz_assert(b);

      // Add it to free list
      free_batches_.AddLast(&b->lln_);

      // Alpha State
      SetState(b->state_);

#if ENABLE_BUFFER_SHARE
      switch(b->state_.vertex_format_) {
        case OpenGLVertexFormat::XYZC:
          glVertexAttribPointer(0, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(OpenGLVertexXYZC), (void*)(offsetof(OpenGLVertexXYZC,c)+b->vertex_offset_));
          glVertexAttribPointer(1, 3, GL_FLOAT,         GL_FALSE, sizeof(OpenGLVertexXYZC), (void*)(offsetof(OpenGLVertexXYZC,xyz)+b->vertex_offset_));
          break;

        case OpenGLVertexFormat::XYZUVC:
          glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, sizeof(OpenGLVertexXYZUVC), (void*)(offsetof(OpenGLVertexXYZUVC,uv)+b->vertex_offset_));
          glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(OpenGLVertexXYZUVC), (void*)(offsetof(OpenGLVertexXYZUVC,c)+b->vertex_offset_));
          glVertexAttribPointer(2, 3, GL_FLOAT,         GL_FALSE, sizeof(OpenGLVertexXYZUVC), (void*)(offsetof(OpenGLVertexXYZUVC,xyz)+b->vertex_offset_));
          break;
          
        default: noz_assert(false); break;
      }
#endif

      if(!(b->state_.flags_ & OpenGLState::FlagDebug)) {
        counter_batches++;
      }

      // Does the active state have an index buffer?
      if(ibsize>0) {
        glDrawElements(b->state_.primitive_type_,b->index_count_,GL_UNSIGNED_SHORT,(GLvoid*)(b->index_offset_*sizeof(noz_uint16)));

      // No index buffer..
      } else {
        glDrawArrays(b->state_.primitive_type_,b->vertex_offset_,b->vertex_count_);
      }
    }
  }

  if(ogl_debug_counters->GetValue()) {
    Console::WriteLine("OpenGLRenderContext:  b:%d  v:%d  i:%d  bw:%d (%d/%d)",
      counter_batches,
      counter_verticies,
      counter_indicies,
      counter_buffer_write,
      counter_buffer_write_used,
      counter_buffer_write_total
    );
  }

#if !ENABLE_REUSE_BATCHES
  while(!free_batches_.IsEmpty()) {OpenGLBatch* b = free_batches_.GetFirst()->GetValue(); free_batches_.RemoveFirst(); delete b;}
#endif

  noz_assert(batches_.IsEmpty());
}

void OpenGLRenderContext::DrawDebug(void) {
  for(auto it=batches_.GetFirst(); it; it=it->GetNext()) {
    if(!(it->GetValue()->state_.flags_ & OpenGLState::FlagDebug)) {
      DrawDebugRectangle(
        Rect(
          it->GetValue()->bounds_.x,
          it->GetValue()->bounds_.y,
          it->GetValue()->bounds_.w - 1,
          it->GetValue()->bounds_.h - 1
        ),        
        Color::Green);
    }
  }
}

void OpenGLRenderContext::DrawDebugLine (const Vector2& v1, const Vector2& v2, Color color) {
  OpenGLState state;
  state.flags_ = OpenGLState::FlagDebug;
  state.primitive_type_ = GL_LINES;
  state.program_ = program_color_;
  state.vertex_format_ = OpenGLVertexFormat::XYZC;
  state.texture_[0] = nullptr;
  state.texture_[1] = nullptr;
  state.stencil_depth_ = stencil_depth_;
  state.stencil_id_ = stencil_id_;

  if(color.a < 255) state.flags_ |= OpenGLState::FlagAlpha;

  static const noz_uint16 indicies[2] = { 0, 1 };

  OpenGLVertexXYZC vert[2];
  vert[0].xyz = PixelCorrect(transform_ * v1);
  vert[1].xyz = PixelCorrect(transform_ * v2);
  vert[0].c = color.raw;
  vert[1].c = color.raw;

  Rect bounds(vert[0].xyz);
  bounds = bounds.Union(vert[1].xyz);

  OpenGLBatch* b = GetBatch(state, bounds, 2, 2);
  if(b) {
    b->Add(vert,2,indicies,2);
  }
}

void OpenGLRenderContext::DrawDebugRectangle (const Rect& r, Color color) {
  OpenGLState state;
  state.flags_ = OpenGLState::FlagDebug;
  state.primitive_type_ = GL_LINES;
  state.program_ = program_color_;
  state.vertex_format_ = OpenGLVertexFormat::XYZC;
  state.texture_[0] = nullptr;
  state.texture_[1] = nullptr;
  state.stencil_depth_ = stencil_depth_;
  state.stencil_id_ = stencil_id_;
  
  Vector3 w (transform_.d[0] * r.w,transform_.d[1] * r.w, 0);
  Vector3 h (transform_.d[3] * r.h,transform_.d[4] * r.h, 0);

  OpenGLVertexXYZC verts[4];
  verts[0].xyz = PixelCorrect(transform_ * Vector2(r.x,r.y));
  verts[1].xyz = verts[0].xyz + w;
  verts[2].xyz = verts[0].xyz + w + h;
  verts[3].xyz = verts[0].xyz + h;

  verts[0].c = color.raw;
  verts[1].c = color.raw;
  verts[2].c = color.raw;
  verts[3].c = color.raw;

  Rect bounds (Vector2(verts[0].xyz));
  bounds = bounds.Union(Vector2(verts[1].xyz));
  bounds = bounds.Union(Vector2(verts[2].xyz));
  bounds = bounds.Union(Vector2(verts[3].xyz));  

  static const noz_uint16 indicies[8] = {0, 1,  1, 2,  2, 3,  3, 0};

  OpenGLBatch* b = GetBatch(state, bounds, 4, 8);
  if(b) {
    b->Add(verts,4,indicies,8);
  }
}

OpenGLTexture* OpenGLRenderContext::GetTexture(Image* image) {
  return (OpenGLTexture*)image->GetHandle();
}

void OpenGLRenderContext::SetTransform(const Matrix3& matrix) {
  transform_ = matrix;
}

OpenGLBatch* OpenGLRenderContext::GetBatch(const OpenGLState& state, const Rect& r, noz_uint32 vc, noz_uint32 ic) {
#if ENABLE_AUTO_BATCHING
  // Scan the open batches for a match.
  for(auto it=batches_.GetLast(); it!=nullptr; it=it->GetPrev()) {
    OpenGLBatch* b = it->GetValue();
    noz_assert(b);

    // Are the states a match?
    bool states_match = b->state_ == state;

    // If the states do not match but there is an intersection then there is 
    // no suitable match for this state
    if(!states_match && r.Intersects(b->bounds_)) {
      break;
    }

    // We found a match!
    if(states_match) {
      if((b->vertex_count_ + vc) * b->vertex_size_ > vertex_buffer_.size()) break;
      if((b->index_count_ + ic) > index_buffer_.size()) break;

      // Add in the rect.
      b->bounds_ = b->bounds_.Union(r);
      return b;
    }
  }
#endif

  // If we are here there was no match so a new batch is required
  OpenGLBatch* b;
  if(free_batches_.IsEmpty()) {
    b = new OpenGLBatch;
  } else {
    b = free_batches_.GetFirst()->GetValue();
    free_batches_.RemoveFirst();
  }

  switch(state.vertex_format_) {
    case OpenGLVertexFormat::XYZC: b->vertex_size_ = sizeof(OpenGLVertexXYZC); break;
    case OpenGLVertexFormat::XYZUVC: b->vertex_size_ = sizeof(OpenGLVertexXYZUVC); break;
    default:
      noz_assert(false);
      break;
  }

  b->vertex_count_ = 0;
  b->vertex_offset_ = 0;
  b->index_count_ = 0;
  b->index_offset_ = 0;
  b->state_ = state;
  b->bounds_ = r;
  batches_.AddLast(&b->lln_);

  return b;
}

void OpenGLBatch::Add (const noz_float* v, noz_uint32 vc, noz_uint32 vs, const noz_uint16* i, noz_uint32 ic) {  
  noz_assert(v);
  noz_assert(vc);
  noz_assert(vertex_size_ == vs);

  // Grow to accomidate the new data
  noz_uint32 size = (vertex_count_ + vc) * vertex_size_;
  if(size > vertex_data_.size()) {
    vertex_data_.resize(size);
  }

  // Copy in the data.
  memcpy(
    &vertex_data_[vertex_count_*vertex_size_],
    v,
    vc*vertex_size_
  );
  
  // Copy the indicies
  if(ic) {
    if(index_count_ + ic > index_data_.size()) {
      index_data_.resize(index_count_ + ic);
    }
    for(noz_uint32 j=0;j<ic;j++) {
      index_data_[index_count_+j] = vertex_count_ + i[j];
    }
  }

  index_count_ += ic;
  vertex_count_ += vc;
}

void OpenGLRenderContext::SetState(const OpenGLState& state) {
  static const noz_byte stencil_depth_write_mask[] = { 0, 1, 2, 4,  8, 16, 32,  64, 128 };
  static const noz_byte stencil_depth_test_mask[] =  { 0, 1, 3, 7, 15, 31, 63, 127, 255 };

  // Update alpha flag
  if((current_state_.flags_ & OpenGLState::FlagAlpha) != (state.flags_&OpenGLState::FlagAlpha)) {
    if(state.flags_ & OpenGLState::FlagAlpha) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
      glDisable(GL_BLEND);
    }
  }

  if(current_state_.stencil_depth_ != state.stencil_depth_) {
    if(state.stencil_depth_==0 && current_state_.stencil_depth_!=0) {
      glDisable(GL_STENCIL_TEST);      
    } else if (current_state_.stencil_depth_==0 && state.stencil_depth_ != 0) {
      glEnable(GL_STENCIL_TEST); 
    }

    if(!(state.flags_&OpenGLState::FlagStencil) && state.stencil_depth_>0) {
      glStencilFunc(GL_EQUAL,stencil_depth_test_mask[state.stencil_depth_], 0xFF);
    }
  }

  if((current_state_.stencil_id_ != state.stencil_id_) || (current_state_.flags_ & OpenGLState::FlagStencil) != (state.flags_&OpenGLState::FlagStencil)) {
    if(state.flags_ & OpenGLState::FlagStencil) {
      if(state.stencil_depth_==1) {
        glClearStencil(0);
        glStencilMask(0xFF);
        glClear(GL_STENCIL_BUFFER_BIT);        
      }

      // Disable writing to color 
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      
      // Disable writing to depth
      glDepthMask(GL_FALSE);

#if !defined(NOZ_IOS)
      // Alpha test when writing
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GEQUAL,1);
#endif

      // Write the stencil depth into the stencil buffer.
      glStencilFunc(GL_NEVER, 0xFF, 0xFF);
      glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
      glStencilMask(stencil_depth_write_mask[state.stencil_depth_]);
    } else {
      glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
      glDepthMask(GL_TRUE);
      
#if !defined(NOZ_IOS)
      glDisable(GL_ALPHA_TEST);
#endif      
      glStencilMask(0x00);
      glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
      glStencilFunc(GL_EQUAL,stencil_depth_test_mask[state.stencil_depth_], 0xFF);
    }
  }

  // Texture?
  if(state.texture_[0] && state.texture_[0] != current_state_.texture_[0]) {
    if(state.texture_[0]) state.texture_[0]->Bind();
  }

  // Linear Filter?
  if(state.texture_[0]) {
    if(state.flags_ & OpenGLState::FlagLinearFilterOverride) {
      state.texture_[0]->SetFilterMode(ImageFilterMode::Linear);
    } else if(state.texture_[0]->current_filter_ != state.texture_[0]->image_->GetFilterMode()) {
      state.texture_[0]->SetFilterMode(state.texture_[0]->image_->GetFilterMode());
    }
  }

  // Program
  if(current_state_.program_ != state.program_) {
    state.program_->use();
  }

  // Vertex format
  if(current_state_.vertex_format_ != state.vertex_format_) {
    switch(state.vertex_format_) {
      case OpenGLVertexFormat::XYZC:
        glDisableVertexAttribArray(2);
#if !ENABLE_BUFFER_SHARE || !ENABLE_BUFFER_SHARE_FORMATS
        glVertexAttribPointer(0, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(OpenGLVertexXYZC), (void*)(offsetof(OpenGLVertexXYZC,c)));
        glVertexAttribPointer(1, 3, GL_FLOAT,         GL_FALSE, sizeof(OpenGLVertexXYZC), (void*)(offsetof(OpenGLVertexXYZC,xyz)));
#endif
        break;

      case OpenGLVertexFormat::XYZUVC:
        glEnableVertexAttribArray(2);
#if !ENABLE_BUFFER_SHARE || !ENABLE_BUFFER_SHARE_FORMATS
        glVertexAttribPointer(0, 2, GL_FLOAT,         GL_FALSE, sizeof(OpenGLVertexXYZUVC), (void*)(offsetof(OpenGLVertexXYZUVC,uv)));
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(OpenGLVertexXYZUVC), (void*)(offsetof(OpenGLVertexXYZUVC,c)));
        glVertexAttribPointer(2, 3, GL_FLOAT,         GL_FALSE, sizeof(OpenGLVertexXYZUVC), (void*)(offsetof(OpenGLVertexXYZUVC,xyz)));
#endif
        break;

      default:
        noz_assert(false);
        break;
    }
  }

  // Set new state
  current_state_ = state;
}

bool OpenGLRenderContext::PushMask (const Rect& rect, Image* mask, Rect& render_rect) {
  NOZ_TODO("Nested image clips are not working");

  if(stencil_depth_==8) return false;

  if(stencil_depth_==0) {
    stencil_id_++;
  }

  stencil_depth_++;      

  Vector3 w (transform_.d[0] * rect.w,transform_.d[1] * rect.w, 0);
  Vector3 h (transform_.d[3] * rect.h,transform_.d[4] * rect.h, 0);

  OpenGLState state;
  state.flags_ = OpenGLState::FlagStencil;
  state.primitive_type_ = GL_TRIANGLES;
  state.texture_[0] = nullptr;
  state.texture_[1] = nullptr;
  state.stencil_depth_ = stencil_depth_;
  state.stencil_id_  = stencil_id_;

  static const noz_uint16 indicies[6] = {
    0, 1, 2, 1, 3, 2
  };

  if(nullptr == mask) {
    state.vertex_format_ = OpenGLVertexFormat::XYZC;
    state.program_ = program_color_;

    OpenGLVertexXYZC verts[4];
    verts[0].xyz = PixelCorrect(transform_ * Vector2(rect.x,rect.y));
    verts[1].xyz = verts[0].xyz + w;
    verts[2].xyz = verts[0].xyz + h;
    verts[3].xyz = verts[0].xyz + w + h;

    verts[0].c = Color::White.raw;
    verts[1].c = Color::White.raw;
    verts[2].c = Color::White.raw;
    verts[3].c = Color::White.raw;

    Rect bounds (Vector2(verts[0].xyz));
    bounds = bounds.Union(Vector2(verts[1].xyz));
    bounds = bounds.Union(Vector2(verts[2].xyz));
    bounds = bounds.Union(Vector2(verts[3].xyz));  
    render_rect = bounds;

    // Find a batch.
    OpenGLBatch* b = GetBatch(state, bounds, 4, 6);
    if(b) {
      // Add the data to the batch.
      b->Add(&verts[0], 4, indicies, 6);
    }
  } else {
    state.vertex_format_ = OpenGLVertexFormat::XYZUVC;

    OpenGLVertexXYZUVC verts[4];
    verts[0].xyz = PixelCorrect(transform_ * Vector2(rect.x,rect.y));
    verts[1].xyz = verts[0].xyz + w;
    verts[2].xyz = verts[0].xyz + h;
    verts[3].xyz = verts[0].xyz + w + h;
  
    verts[0].uv.set(0,0);
    verts[1].uv.set(1,0);
    verts[2].uv.set(0,1);
    verts[3].uv.set(1,1);

    verts[0].c = Color::White.raw;
    verts[1].c = Color::White.raw;
    verts[2].c = Color::White.raw;
    verts[3].c = Color::White.raw;

    Rect bounds (Vector2(verts[0].xyz));
    bounds = bounds.Union(Vector2(verts[1].xyz));
    bounds = bounds.Union(Vector2(verts[2].xyz));
    bounds = bounds.Union(Vector2(verts[3].xyz));  
    render_rect = bounds;

    state.texture_[0] = GetTexture(mask);
    
    switch(mask->GetFormat()) {
      case ImageFormat::A8:
        state.program_ = program_texture_a8_;
        state.flags_ |= OpenGLState::FlagAlpha;
        break;

      case ImageFormat::SDF:
        state.program_ = program_texture_sdf_;
        state.flags_ |= OpenGLState::FlagLinearFilterOverride;
        state.flags_ |= OpenGLState::FlagAlpha;
        break;

      case ImageFormat::R8G8B8A8: 
        state.program_ = program_texture_;
        state.flags_ |= OpenGLState::FlagAlpha;
        break;

      default:
        state.program_ = program_texture_;
        break;        
    }

    // Find a batch.
    OpenGLBatch* b = GetBatch(state, bounds, 4, 6);
    if(b) {
      // Add the data to the batch.
      b->Add(&verts[0], 4, indicies, 6);
    }
  }

  return true;
}

void OpenGLRenderContext::PopMask (void) {
  stencil_depth_--;
}


bool OpenGLRenderContext::Draw (RenderMesh* mesh, noz_float opacity) {
  OpenGLState state;
  state.flags_ = 0;
  state.primitive_type_ = GL_TRIANGLES;
  state.texture_[0] = nullptr;
  state.texture_[1] = nullptr;
  state.stencil_depth_ = stencil_depth_;
  state.stencil_id_ = stencil_id_;

  noz_uint32 rvc = mesh->GetVerticies().size();
  noz_uint32 ric = mesh->GetTriangles().size() * 3;

  if(rvc==0 || ric==0) return false;

  const RenderMesh::Vertex* rv = &mesh->GetVerticies()[0];
  const noz_uint16* ri = &mesh->GetTriangles()[0].index[0];

  Rect bounds;

  NOZ_TODO("check for culling off screen");

  // If the state has an image..
  if(mesh->GetImage()) {
    state.vertex_format_ = OpenGLVertexFormat::XYZUVC;
    state.texture_[0] = GetTexture(mesh->GetImage());
    state.program_ = GetProgramForImage(mesh->GetImage());
    state.flags_ |= GetFlagsForImage(mesh->GetImage());

    // Automatic filtering sets the linear override flag if the image is scaled or rotated in any way.
    if(mesh->GetImage()->GetFilterMode() == ImageFilterMode::Automatic) {
      if(!((transform_.d[0] >= -0.000001f && transform_.d[0] <= 0.000001f) || 
           (transform_.d[0] >=  0.999999f && transform_.d[0] <= 1.000001f)) ||
         !((transform_.d[4] >= -0.000001f && transform_.d[4] <= 0.000001f) || 
           (transform_.d[4] >=  0.999999f && transform_.d[4] <= 1.000001f))   ) {
        state.flags_ |= OpenGLState::FlagLinearFilterOverride;
      }
    }

    std::vector<OpenGLVertexXYZUVC> verts;
    verts.resize(rvc);
    unsigned char alpha_min = 255;
    for(noz_uint32 v=0;v<verts.size();v++) {
      verts[v].xyz = PixelCorrect(transform_ * rv[v].xy);
      verts[v].c = rv[v].color.ModulateAlpha(opacity).raw;
      verts[v].uv = rv[v].uv;
      alpha_min &= rv[v].color.a;
    }

    alpha_min = (noz_byte)(opacity * alpha_min);

    if(alpha_min < 255) state.flags_ |= OpenGLState::FlagAlpha;

    bounds.x = verts[0].xyz.x;
    bounds.y = verts[1].xyz.y;
    for(noz_uint32 v=0; v<verts.size(); v++) {
      bounds = bounds.Union(verts[v].xyz);
    }

    OpenGLBatch* b = GetBatch(state, bounds, rvc, ric);
    if(b) {
      b->Add(&verts[0],verts.size(),(noz_uint16*)ri,ric);
    }
  } else {
    state.vertex_format_ = OpenGLVertexFormat::XYZC;
    state.program_ = program_color_;

    std::vector<OpenGLVertexXYZC> verts;
    verts.resize(rvc);
    unsigned char alpha_min = 255;
    for(noz_uint32 v=0;v<rvc;v++) {
      verts[v].xyz = PixelCorrect(transform_ * rv[v].xy);
      verts[v].c = rv[v].color.ModulateAlpha(opacity).raw;
      alpha_min &= rv[v].color.a;
    }

    if(alpha_min < 255) state.flags_ |= OpenGLState::FlagAlpha;

    bounds.x = verts[0].xyz.x;
    bounds.y = verts[1].xyz.y;
    for(noz_uint32 v=1; v<verts.size(); v++) {
      bounds = bounds.Union(verts[v].xyz);
    }

    OpenGLBatch* b = GetBatch(state, bounds, rvc, ric);
    if(b) {
      b->Add(&verts[0],verts.size(),(noz_uint16*)ri,ric);
    }
  }

  return true;
}

OpenGLProgram* OpenGLRenderContext::GetProgramForImage (Image* image) {
  switch(image->GetFormat()) {
    case ImageFormat::A8:  return program_texture_a8_;
    case ImageFormat::SDF: return program_texture_sdf_;
    default:
      break;        
  }

  return program_texture_;
}

noz_uint32 OpenGLRenderContext::GetFlagsForImage (Image* image) {
  switch(image->GetFormat()) {
    case ImageFormat::A8:  return OpenGLState::FlagAlpha;
    case ImageFormat::SDF: return OpenGLState::FlagAlpha | OpenGLState::FlagLinearFilterOverride;
    case ImageFormat::R8G8B8A8: return OpenGLState::FlagAlpha;
    default:
      break;        
  }

  return 0;
}


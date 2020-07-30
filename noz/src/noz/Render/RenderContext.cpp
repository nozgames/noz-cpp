///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Platform/RenderContextHandle.h>
#include "RenderContext.h"

using namespace noz;
using namespace noz::Platform;


RenderContext::RenderContext(void) {
  matrix_stack_.push_back(Matrix3::Identity);
  opacity_stack_.push_back(1.0f);
  handle_ = Platform::RenderContextHandle::CreateInstance();
  own_handle_ = true;
}

RenderContext::RenderContext(RenderContextHandle* handle) {
  matrix_stack_.push_back(Matrix3::Identity);
  opacity_stack_.push_back(1.0f);
  handle_ = handle;  
  own_handle_ = false;
}

RenderContext::~RenderContext(void) {
  if(own_handle_) delete handle_;
}

void RenderContext::Begin (const Vector2& size, RenderTarget* target) {
  handle_->Begin(size, target);
}

void RenderContext::End (void) {
  handle_->End();
}

void RenderContext::DrawDebugLine(const Vector2& v1, const Vector2& v2, Color color) {
  noz_assert(handle_);
  handle_->DrawDebugLine(v1,v2,color);
}

void RenderContext::PushMatrix (void) {
  matrix_stack_.push_back(matrix_stack_[matrix_stack_.size()-1]);
  states_.back().matrix_count_++;
}

void RenderContext::MultiplyMatrix(const Matrix3& mat) {
  Matrix3& back = matrix_stack_[matrix_stack_.size()-1];
  back = mat * back;
  handle_->SetTransform(back);
}

void RenderContext::SetMatrix(const Matrix3& mat) {
  Matrix3& back = matrix_stack_[matrix_stack_.size()-1];
  back = mat;
  handle_->SetTransform(back);
}

bool RenderContext::PushMask(const Rect& clip,Image* mask, Rect& render_rect){
  noz_assert(handle_);
  bool result = handle_->PushMask(clip,mask,render_rect);
  states_.back().mask_count_++;
  return result;
}

void RenderContext::PopMask(void) {
  noz_assert(handle_);
  handle_->PopMask();
  states_.back().mask_count_--;
}

void RenderContext::PopMatrix (void) {
  noz_assert(matrix_stack_.size()>1);
  noz_assert(states_.back().matrix_count_>0);
  matrix_stack_.pop_back();
  handle_->SetTransform(matrix_stack_[matrix_stack_.size()-1]);
  states_.back().matrix_count_--;
}

bool RenderContext::Draw (RenderMesh* mesh) {
  noz_assert(handle_);
  return handle_->Draw(mesh,opacity_stack_.back());
}

void RenderContext::PushState(void) {
  State state;
  state.mask_count_ = 0;
  state.matrix_count_ = 0;
  states_.push_back(state);
}

void RenderContext::PopState(void) {
  noz_assert(!states_.empty());

  State& state = states_.back();

  // Pop any outstanding masks
  while(state.mask_count_) PopMask();

  // Pop any outstanding matricies
  for(;state.matrix_count_;state.matrix_count_--) matrix_stack_.pop_back();
  handle_->SetTransform(matrix_stack_[matrix_stack_.size()-1]);

  states_.pop_back();
}

void RenderContext::PushOpacity (noz_float opacity) {
  opacity_stack_.push_back(opacity_stack_.back() * opacity);
}

void RenderContext::PopOpacity (void) {
  opacity_stack_.pop_back();
}


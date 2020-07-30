//////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Platform/ImageHandle.h>
#include <noz/Serialization/Serializer.h>
#include <noz/Serialization/Deserializer.h>

using namespace noz;
using namespace noz::Platform;

Image::Image(void) {
  width_ = 0;
  height_ = 0;
  locked_ = nullptr;
  format_ = ImageFormat::Unknown;
  handle_ = ImageHandle::CreateInstance(this);
  wrap_mode_ = ImageWrapMode::Clamp;
  filter_mode_ = ImageFilterMode::None;
}

Image::Image (noz_int32 width, noz_int32 height, ImageFormat format) {
  width_ = width;
  height_ = height;
  format_ = format;
  locked_ = nullptr;
  wrap_mode_ = ImageWrapMode::Clamp;
  filter_mode_ = ImageFilterMode::None;
  handle_ = ImageHandle::CreateInstance(this);
}

Image::~Image(void) {
  delete handle_;
}

noz_byte* Image::Lock (void) {
  if(locked_) return nullptr; 
  locked_ = handle_->Lock();
  return locked_;
}

void Image::Unlock(void) {
  if(!locked_) return;
  locked_ = nullptr;
  handle_->Unlock();
}


bool Image::SerializeBuffer (Serializer& s) {
  noz_byte* b = Lock();
  if(nullptr == b) {
    s.WriteValueNull();
    return true;
  }

  s.WriteStartArray();
  s.WriteValueInt32(width_);
  s.WriteValueInt32(height_);
  s.WriteValueName(Enum<ImageFormat>::GetName(format_));
  s.WriteValueBytes(b,GetHeight() * GetStride());
  s.WriteEndArray();

  Unlock();

  return true;
}

bool Image::DeserializeBuffer (Deserializer& s) {
  if(s.PeekValueNull()) {
    return true;
  }

  if(!s.ReadStartArray()) return false;
  if(!s.ReadValueInt32(width_)) return false;
  if(!s.ReadValueInt32(height_)) return false;

  Name format_name;
  if(!s.ReadValueName(format_name)) return false;
  format_ = Enum<ImageFormat>::GetValue(format_name);

  noz_byte* b = Lock();
  noz_uint32 size = 0;
  if(!s.ReadValueBytesSize(size)) return false;
  if(size != GetHeight() * GetStride()) return false;
  if(!s.ReadValueBytes(b)) return false;
  Unlock();

  return s.ReadEndArray();
}

void Image::SetWrapMode (ImageWrapMode wrap) {
  if(wrap_mode_ == wrap) return;
  wrap_mode_ = wrap;
  handle_->InvalidateState();
}

void Image::SetFilterMode (ImageFilterMode mode) {
  if(filter_mode_ == mode) return;
  filter_mode_ = mode;
  handle_->InvalidateState();
}


void Image::SetFrom (Image* image) {
  width_ = image->width_;
  height_ = image->height_;
  format_ = image->format_;
  wrap_mode_ = image->wrap_mode_;
  filter_mode_ = image->filter_mode_;

  delete handle_;
  handle_ = ImageHandle::CreateInstance(this);

  noz_byte* old_locked = image->Lock();
  noz_byte* new_locked = handle_->Lock();
  memcpy(new_locked, old_locked, GetSizeInBytes());
  image->Unlock();
  handle_->Unlock();
}

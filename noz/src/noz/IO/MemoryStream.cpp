///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryWriter.h>
#include <noz/IO/BinaryReader.h>
#include "MemoryStream.h"

using namespace noz;

MemoryStream::MemoryStream(void) {
  position_ = 0;
  length_= 0;
  external_buffer_ = false;
  capacity_ = 0;
  buffer_ = nullptr;
}

MemoryStream::MemoryStream(noz_uint32 capacity) {
  position_ = 0;
  length_= 0;
  external_buffer_ = false;
  capacity_ = capacity;
  if(capacity>0) {
    buffer_ = new noz_byte[capacity];
  } else {
    buffer_ = nullptr;
  }
}

MemoryStream::MemoryStream(noz_byte* bytes, noz_uint32 size) {
  position_ = 0;
  length_= size;
  external_buffer_ = true;
  capacity_ = size;
  buffer_ = bytes;
}

MemoryStream::~MemoryStream(void) {
  if(!external_buffer_) {
    delete[] buffer_;
  }
}

noz_uint32 MemoryStream::Seek(noz_int32 offset, SeekOrigin origin) {
  noz_int32 new_position = position_;
  switch(origin) {
    case SeekOrigin::Begin:   new_position=offset; break;
    case SeekOrigin::Current: new_position+=offset; break;
    case SeekOrigin::End:     new_position=length_-offset; break;
  }

  if(new_position<=0) {
    position_ = 0;
  } else {
    position_ = Math::Min((noz_uint32)new_position,length_);
  }

  return position_;
}

noz_int32 MemoryStream::Read(char* buffer, noz_int32 offset, noz_int32 count) {
  // Clamp count...
  count = Math::Min(length_-position_, (noz_uint32)count);

  // Copy buffer 
  if(count>0) {
    memcpy(buffer+offset,buffer_+position_,count);
  }

  position_ += count;

  return count;
}

noz_int32 MemoryStream::Write(char* buffer, noz_int32 offset, noz_int32 count) {  
  // Would this write go past the end of our capacity?
  if(position_ + count > noz_int32(capacity_)) {
    // Double buffer size if out of room.
    if(!external_buffer_) {
      noz_int32 new_capacity = Math::Max(1,(noz_int32)capacity_);
      while(position_ + count > (noz_uint32)new_capacity) {
        new_capacity*=2;
      }
      SetCapacity(new_capacity);
    } else {
      count = Math::Min(count,(noz_int32)(capacity_ - position_));
    }
  }

  if(count<=0) return 0;

  memcpy(buffer_+position_, buffer+offset, count);
  position_ += count;
  length_ = Math::Max(length_,position_);

  return count;
}

void MemoryStream::SetLength(noz_int32 length) {
  if(length > (noz_int32)capacity_) {
    SetCapacity(length);
  }

  length_ = length;
  position_ = Math::Min(length_,position_);
}

void MemoryStream::SetCapacity(noz_int32 capacity) {
  if(capacity < (noz_int32)capacity_) {
    return;
  }

  noz_byte* new_buffer = new noz_byte[capacity];
  if(buffer_) {
    memcpy(new_buffer,buffer_,length_);
    delete [] buffer_;
  }
  buffer_ = new_buffer;
  capacity_ = capacity;
}

void MemoryStream::ShrinkToFit(void) {
  noz_byte* new_buffer = new noz_byte[length_];
  if(buffer_) {
    memcpy(new_buffer,buffer_,length_);
    delete [] buffer_;
  }
  buffer_ = new_buffer;
  capacity_ = length_;
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AlphaMapFilter.h"

using namespace noz;

Image* AlphaMapFilter::Filter (Image* image) {
  noz_assert(image);

  // Cache width / height / stride
  noz_uint32 w = image->GetWidth();
  noz_uint32 h = image->GetHeight();

  Image* result = new Image(image->GetWidth(), image->GetHeight(), ImageFormat::A8);

  noz_byte* in = image->Lock();
  noz_byte* out = result->Lock();

  // Straight copy if input is already an alpha map
  if(image->GetFormat() == ImageFormat::A8 || image->GetFormat() == ImageFormat::SDF) {
    image->Unlock();
    result->Unlock();
    memcpy(out,in,w*h);
    return result;
  }

  // If source image has no alpha map just return empty alpha
  if(image->GetFormat() != ImageFormat::R8G8B8A8) {
    image->Unlock();
    result->Unlock();
    memset(out,0,w*h);
    return result;
  }

  // Extract alpha
  in += 3;
  for(noz_uint32 y=0;y<h;y++) {
    for(noz_uint32 x=0;x<w;x++,in+=4,out++) {
      *out = *in;
    }
  }

  image->Unlock();
  result->Unlock();

  return result;
}

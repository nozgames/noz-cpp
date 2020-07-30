///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ResizeFilter.h"

using namespace noz;


Image* ResizeFilter::Filter (Image* image) {
  noz_assert(image);

  // Cache width / height / stride
  noz_uint32 w = image->GetWidth();
  noz_uint32 h = image->GetHeight();

  Image* result = new Image(width_,height_, image->GetFormat());

  noz_byte* out = result->Lock();

  if(height_>1 && width_>1) {
    noz_uint32 depth = image->GetDepth();
    for(noz_uint32 y=0;y<(noz_uint32)height_;y++) {
      for(noz_uint32 x=0;x<(noz_uint32)width_;x++) {
        for(noz_uint32 d=0;d<depth;d++,out++) {
          *out = SampleBilinear(image,x,y,d,width_,height_);
        }
      }
    }  
  }

  result->Unlock();

  return result;
}

noz_byte ResizeFilter::SampleBilinear(Image* image, noz_int32 sx, noz_int32 sy, noz_int32 sd, noz_int32 sw, noz_int32 sh) {
  // Calculate the ratio of the new image size to old image size
  noz_float tx = (noz_float)image->GetWidth() / (noz_float)(sw); 
  noz_float ty = (noz_float)image->GetHeight() / (noz_float)(sh); 

  noz_int32 x = (noz_int32)(tx * sx);
  noz_int32 y = (noz_int32)(ty * sy);
  noz_int32 xd = (noz_int32)(tx * (sx+1)) - x;
  noz_int32 yd = (noz_int32)(ty * (sy+1)) - y;
 
  noz_float x_diff = ((tx * sx) - x);
  noz_float y_diff = ((ty * sy) - y);
  
  noz_int32 a = y*image->GetStride() + x * image->GetDepth() + sd;
  noz_int32 b = a + xd * image->GetDepth();
  noz_int32 c = a + yd * image->GetStride();
  noz_int32 d = c + xd * image->GetDepth() ;

  noz_byte* buffer = image->Lock();

  noz_float f =     
    buffer[a] * (1.0f-x_diff)*(1.0f-y_diff) +
    buffer[b] * (x_diff)*(1.0f-y_diff) +
    buffer[c] * (1.0f-x_diff)*(y_diff) +
    buffer[d] * (x_diff)*(y_diff);

  image->Unlock();

  return (noz_byte)(f);
}
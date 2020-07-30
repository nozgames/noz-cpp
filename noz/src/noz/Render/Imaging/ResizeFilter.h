///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ResizeFilter_h__
#define __noz_ResizeFilter_h__

#include "ImageFilter.h"

namespace noz {

  /// Creates a signed distance field from the given image.
  class ResizeFilter : public ImageFilter {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Width)
    private: noz_int32 width_;

    NOZ_PROPERTY(Name=Height)
    private: noz_int32 height_;

    /// Execute the image filter..
    public: virtual Image* Filter (Image* image) override;

    public: void SetWidth(noz_int32 w) {width_=w;}
    public: void SetHeight(noz_int32 h) {height_=h;}

    private: noz_byte SampleBilinear(Image* image, noz_int32 sx, noz_int32 sy, noz_int32 sd, noz_int32 sw, noz_int32 sh);
  };

} // namespace noz


#endif //__noz_ResizeFilter_h__


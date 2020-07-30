///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SignedDistanceFieldFilter_h__
#define __noz_SignedDistanceFieldFilter_h__

#include "ImageFilter.h"

namespace noz {

  /// Creates a signed distance field from the given image.
  class SignedDistanceFieldFilter : public ImageFilter {
    NOZ_OBJECT()

    private: struct Pixel {
      noz_float dx_;
      noz_float dy_;

      //noz_int32 DistanceSqr (void) const {return Math::Max(dx_*dx_,dy_*dy_); }
      noz_float DistanceSqr (void) const {return dx_*dx_ + dy_*dy_; }
    };

    private: struct PixelMap {
      Pixel* p_;
      noz_uint32 w_;
      noz_uint32 h_;

      Pixel* Get (noz_uint32 x, noz_uint32 y) const {return p_ + y * w_ + x;}
    };

    /// Execute the image filter..
    public: virtual Image* Filter (Image* image) override;

    private: void Compute (PixelMap* pm);
    private: void Compute (PixelMap* pm, Pixel* p, noz_uint32 x, noz_uint32 y, noz_int32 ox, noz_int32 oy);
  };

} // namespace noz


#endif //__noz_SignedDistanceFieldFilter_h__



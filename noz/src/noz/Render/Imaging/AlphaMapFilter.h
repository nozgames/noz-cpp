///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AlphaMapFilter_h__
#define __noz_AlphaMapFilter_h__

#include "ImageFilter.h"

namespace noz {

  /// Creates a signed distance field from the given image.
  class AlphaMapFilter : public ImageFilter {
    NOZ_OBJECT()

    /// Execute the image filter..
    public: virtual Image* Filter (Image* image) override;
  };

} // namespace noz


#endif //__noz_Graphics_Imaging_AlphaMapFilter_h__


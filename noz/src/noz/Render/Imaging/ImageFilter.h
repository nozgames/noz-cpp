///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ImageFilter_h__
#define __noz_ImageFilter_h__

namespace noz {

  class ImageFilter : public Object {
    NOZ_OBJECT(NoAllocator)

    /// Execute the image filter..
    public: virtual Image* Filter (Image* image) = 0;
  };

} // namespace noz

#endif //__noz_ImageFilter_h__


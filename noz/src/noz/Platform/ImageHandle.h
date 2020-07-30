///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_ImageHandle_h__
#define __noz_Platform_ImageHandle_h__

namespace noz {
namespace Platform { 

  class ImageHandle : public Handle {
    public: static ImageHandle* CreateInstance(Image* image);

    public: virtual noz_byte* Lock(void) = 0;
    public: virtual void Unlock(void) = 0;

    public: virtual void InvalidateState (void) = 0;
  };

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_ImageHandle_h__


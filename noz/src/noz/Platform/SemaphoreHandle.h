///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_SemaphoreHandle_h__
#define __noz_Platform_SemaphoreHandle_h__

#include <noz/System/Semaphore.h>

namespace noz {
namespace Platform {

  class SemaphoreHandle {
    public: static SemaphoreHandle* CreateInstance(void);

    public: virtual ~SemaphoreHandle(void) {}
  };        

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_SemaphoreHandle_h__


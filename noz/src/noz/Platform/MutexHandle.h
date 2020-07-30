///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_MutexHandle_h__
#define __noz_Platform_MutexHandle_h__

#include <noz/System/Mutex.h>

namespace noz {
namespace Platform {

  class MutexHandle {
    public: static MutexHandle* CreateInstance(void);

    public: virtual ~MutexHandle (void) {}

    public: virtual bool WaitOne (noz_int32 timeout_ms) = 0;

    public: virtual void Release (void) = 0;
  };        

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_MutexHandle_h__


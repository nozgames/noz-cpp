///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_ProcessHandle_h__
#define __noz_Platform_ProcessHandle_h__

#include <noz/System/Process.h>

namespace noz {
namespace Platform {

  class ProcessHandle {
    public: static ProcessHandle* CreateInstance(const char* path, const char* args);

    public: virtual ~ProcessHandle(void) {}

    public: virtual bool HasExited (void) const = 0;

    public: virtual void WaitForExit (void) = 0;

    public: virtual bool WaitForExit (noz_uint32 milliseconds) = 0;
  };        

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_ProcessHandle_h__


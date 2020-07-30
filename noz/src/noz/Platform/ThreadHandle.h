///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_ThreadHandle_h__
#define __noz_Platform_ThreadHandle_h__

#include <noz/System/Thread.h>

namespace noz {
namespace Platform {

  class ThreadHandle {
    public: static ThreadHandle* CreateInstance(void);

    /// Create the thread.
    public: virtual bool Create (Thread* thread, Thread::Proc proc) = 0;

    public: virtual void Terminate (void) = 0;

    public: virtual bool WaitForExit (noz_int32 milliseconds) = 0;
  };        

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_SocketHandle_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Platform/MutexHandle.h>

#include <Windows.h>

using namespace noz;
using namespace noz::Platform;

namespace noz {
namespace Platform {
namespace Windows {

  class WindowsMutex : public MutexHandle {    
    /// Thread handle  
    private: HANDLE handle_;

    public: WindowsMutex (void) {
      handle_ = ::CreateMutex ( NULL, FALSE, NULL );
    }

    public: ~WindowsMutex (void) {
      CloseHandle (handle_);
    }

    public: bool WaitOne (noz_int32 timeout_ms) {
      return WAIT_OBJECT_0 == ::WaitForSingleObject (handle_, timeout_ms==-1 ? INFINITE : timeout_ms);
    }

    public: void Release (void) {
      ::ReleaseMutex (handle_);
    }
  };

}
}
}

MutexHandle* MutexHandle::CreateInstance (void) {
  return new Windows::WindowsMutex;
}

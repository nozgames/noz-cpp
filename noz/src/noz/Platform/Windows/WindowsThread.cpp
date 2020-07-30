///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Platform/ThreadHandle.h>
#include <Windows.h>

using namespace noz;
using namespace noz::Platform;


namespace noz {
namespace Platform {
namespace Windows {

  static DWORD TLSSlot =TLS_OUT_OF_INDEXES;

  class WindowsThread : public ThreadHandle {    
    /// Associated thread
    private: Thread* thread_;

    /// Thread handle  
    private: HANDLE handle_;

    /// Thread ID
    private: DWORD id_;

    /// Function to call for thread
    private: Thread::Proc proc_;

    /// Thrad local storage
    private: DWORD tls_;

    public: WindowsThread(void) {
      thread_ = nullptr;
      id_ = 0;
      handle_ = INVALID_HANDLE_VALUE;
    }

    public: virtual bool Create(Thread* thread, Thread::Proc proc) override {
      if(TLSSlot==TLS_OUT_OF_INDEXES) {
        TLSSlot=TlsAlloc();
      }

      proc_ = proc;
      thread_ = thread;
     
      // Start the thread
      handle_ = CreateThread ( NULL, 0, ThreadProc, this, STACK_SIZE_PARAM_IS_A_RESERVATION, &id_ );
      if (handle_ == INVALID_HANDLE_VALUE) {        
        return false;
      }
      
      return true;
    }

    public: virtual void Terminate (void) override {
      if(handle_ == INVALID_HANDLE_VALUE) return;
      TerminateThread (handle_,0);
    }

    public: virtual bool WaitForExit (noz_int32 milliseconds) {
      if(handle_ == INVALID_HANDLE_VALUE) return true;
      return WAIT_TIMEOUT!=WaitForSingleObject(handle_,milliseconds);
    }

    static DWORD WINAPI ThreadProc (LPVOID i_data) {
      WindowsThread* winthread = (WindowsThread*)i_data;  
      TlsSetValue(TLSSlot,winthread);
      winthread->proc_ (winthread->thread_);
      return 0;
    }

  };

}
}
}

void Thread::Sleep(noz_int32 milliseconds) {
  ::Sleep (milliseconds);
}

ThreadHandle* ThreadHandle::CreateInstance(void) {
  return new Platform::Windows::WindowsThread;
}
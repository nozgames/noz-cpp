///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Platform/ThreadHandle.h>

#include <Foundation/Foundation.h>
#include <pthread.h>

using namespace noz;
using namespace noz::Platform;


namespace noz {
namespace Platform {
namespace IOS {

  class IOSThread : public ThreadHandle {    
    /// Associated thread
    private: Thread* thread_;

    /// Thread ID
    private: pthread_t id_;

    /// Function to call for thread
    private: Thread::Proc proc_;

    public: IOSThread(void) {
      thread_ = nullptr;
    }

    public: virtual bool Create(Thread* thread, Thread::Proc proc) override {
	  thread_ = thread;
      proc_ = proc;

	  pthread_attr_t attr; 
	  pthread_attr_init(&attr); 
	  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); 

	  pthread_create(&id_, &attr, ThreadProc, this );

	  pthread_attr_destroy(&attr);
      
      return true;
    }

	private: static void* ThreadProc (void* data) {
	  NSAutoreleasePool* autoReleasePool = [NSAutoreleasePool new];

	  IOSThread* iosthread = (IOSThread*) data;
	  //pthread_setspecific(iPhoneThreadSlot,thread);
	  iosthread->proc_ (iosthread->thread_);
  
	  [autoReleasePool release];
  
	  return 0;
	}
  };

}
}
}

void Thread::Sleep(noz_int32 milliseconds) {
  usleep ( milliseconds * 1000 );
}

ThreadHandle* ThreadHandle::CreateInstance(void) {
  return new Platform::IOS::IOSThread;
}

#include <noz/Platform/MutexHandle.h>


MutexHandle* MutexHandle::CreateInstance (void) {return nullptr;}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Thread.h"
#include <noz/Platform/ThreadHandle.h>

using namespace noz;
using namespace noz::Platform;

Thread::Thread (void) {
  id_ = 0;
  handle_ = ThreadHandle::CreateInstance();
}

Thread::~Thread ( void ) {
}

bool Thread::Start (const ThreadStart& start) {
  // TODO: check if already started..

  start_ = start;

  // Create the thread
  if(!handle_->Create(this, ThreadProc)) {
    return false;
  }
  
  return true;
}

void Thread::Terminate (void) {
  noz_assert(handle_);
  handle_->Terminate();
}

void Thread::WaitForExit (void) {
  noz_assert(handle_);
  handle_->WaitForExit(-1);
}

bool Thread::WaitForExit (noz_uint32 milliseconds) {
  noz_assert(handle_);
  return handle_->WaitForExit(milliseconds);
}

noz_uint32 Thread::ThreadProc (Thread* thread) {
  thread->start_();
  return 0;  
}


#if 0
noz_err Thread::stop (int i_wait) {
  if (!is_running()) {
    return ErrorNone;
  }
  
  m_state = State::Stopping;

  on_state (m_state);

  noz_err result = ErrorNone;

  if (i_wait) {
    wait (i_wait);
    result = (m_state == State::Stopped) ? ErrorNone : ErrorTimeout;
  }
  
  return result;
}


noz_err Thread::lock ( void ) {
#if 0
  if ( !is_running ( ) ) {
    return ErrorNotRunning;
  }
  m_lockMutex.get();
#endif  
  return ErrorNone;
}
  

noz_err Thread::unlock ( void ) {
#if 0
  if ( !is_running ( ) ) {
    return ErrorNotRunning;
  }
  m_lockMutex.put();
#endif  
  return ErrorNone;
} 


void Thread::wait (int i_time) {
  if (m_context) {
    Thread_wait (m_context, i_time);
  }
}


void Thread::sleep (int i_time ) {
  Thread_sleep ( m_context, i_time );
}


unsigned int Thread::proc (void* i_data) {
  Thread* thread = (Thread*)i_data;

  thread->m_id = Thread_id ( );

  // Execute the thread and save its return result
  thread->m_error = thread->run();

  // Move the thread into a stopped state
  thread->m_state = State::Stopped;
  thread->on_state (thread->m_state);
  
  return 0;
}


Thread* Thread::current ( void ) {
  return Thread_current();
}


Thread::TLS Thread::tls_alloc(void) {
  return Thread_tls_alloc();
}
    
void Thread::tls_free(TLS i_tls) {
  Thread_tls_free(i_tls);
}
    
void Thread::tls_set(TLS i_tls, void* i_value) {
  Thread_tls_set(i_tls,i_value);
}
    
void* Thread::tls_get(TLS i_tls) {
  return Thread_tls_get(i_tls);
}
#endif

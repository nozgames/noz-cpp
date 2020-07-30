///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Thread_h__
#define __noz_Thread_h__

#include "ThreadStart.h"

namespace noz { namespace Platform { class ThreadHandle; } }

namespace noz {

  class Thread : public Object {
    public: typedef noz_uint32 (*Proc) (Thread* thread);    
    public: typedef void* TLS;
    public: typedef void* Context;    
    public: typedef void* ID;

    /// Platform specific thread implementation
    private: Platform::ThreadHandle* handle_;

    private: ThreadStart start_;

    /// Unique identifier of thread
    private: ID id_;

    /// Identifier of main thread
    private: static ID main_id_;               

    public: Thread ( void );

    public: virtual ~Thread ( void );

    public: ID GetID (void) const {return id_;}
    
    /// Start the thread
    public: bool Start (const ThreadStart& start);

    public: static void Sleep (noz_int32 milliseconds);

    private: static noz_uint32 ThreadProc (Thread* thread);

    public: void Terminate (void);

    public: void WaitForExit (void);

    public: bool WaitForExit (noz_uint32 milliseconds);

/*
    /// Stop the thrad.
    public: noz_err stop (int i_wait=0);

    public: void wait (int i_time=-1);

    public: noz_err lock (void);
    
    public: noz_err unlock (void);

    public: bool is_running (void) const {return state() != State::Stopped;}

    public: State state (void) const {return m_state;}

    public: noz_err error (void) const {return m_error;}
       
    public: void sleep (int i_time);

    public: static TLS tls_alloc(void);
    
    public: static void tls_free(TLS i_tls);
    
    public: static void tls_set(TLS i_tls, void* i_value);
    
    public: static void* tls_get(TLS i_tls);
    
    public: static Thread* current (void);
    

    protected: virtual noz_err run (void) = 0;

   
    protected: virtual void on_state (State i_state) {}
   
    private: static unsigned int proc (void* i_data);
*/
  };


} // namespace noz


#endif //__noz_Thread_h__

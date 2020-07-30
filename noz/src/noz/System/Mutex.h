///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Mutex_h__
#define __noz_Mutex_h__

namespace noz {

  namespace Platform { class MutexHandle; }

  class Mutex {
    /// Platform specific implementation
    private: Platform::MutexHandle* handle_;

    /// Default constructor    
    public: Mutex (void);

    /// Destructor
    public: ~Mutex ( void );
    
    /// Blocks the current thread until the mutex is signaled
    public: bool WaitOne (void);

    /// Blocks the current thread until the mutex is signaled
    public: bool WaitOne (noz_int32 timeout_ms);

    /// Releases the mutex once.
    public: void Release (void);
  };
  

} // namespace noz


#endif // __noz_Mutex_h__

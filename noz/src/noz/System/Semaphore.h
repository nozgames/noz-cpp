///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Semaphore_h__
#define __noz_Semaphore_h__

namespace noz {

  namespace Platform { class SemaphoreHandle; }

  class Semaphore {

    private: Platform::SemaphoreHandle* handle_;

    public: Semaphore (noz_int32 count);
    public: Semaphore (void);    
    public: ~Semaphore (void);
    
    public: void Release (void);
    public: void Release (noz_int32 count);

    public: bool WaitOne (void);
    public: bool WaitOne (noz_int32 timeout_ms);
  };


} // namespace noz


#endif // __noz_Semaphore_h__

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Process_h__
#define __noz_Process_h__

namespace noz { namespace Platform { class ProcessHandle; } }

namespace noz {

  class Process : public Object {
    private: Platform::ProcessHandle* handle_;

    public: Process (void);

    public: ~Process (void);

    public: static Process* Start (const char* path, const char* args="");

    public: bool HasExited (void) const;

    public: void WaitForExit (void);

    public: bool WaitForExit (noz_uint32 milliseconds);
  };

} // namespace noz


#endif // __noz_Process_h__

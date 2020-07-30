///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Debugger_h__
#define __noz_Debugger_h__

#define NOZ_DEBUGGER_HOOK()   \
  if(Debugger::IsActive()) {static Debugger::Hook hook__ (__FILE__,__LINE__,__FUNCTION__); Debugger::ExecuteHook(&hook__,this);}

namespace noz {

  class Debugger {
    public: class Hook {
      public: String file_;
      public: String function_;
      public: noz_int32 line_;
      public: Hook (const char* file, int line, const char* function) {
        file_ = file; function_ = function; line_ = line;
      }
    };

    private: static Debugger* this_;
    
    private: Debugger(void);

    /// Initialize the debugger and make it active
    public: static void Initialize (void);

    /// Uninitialize the debugger deactivating it
    public: static void Uninitialize (void);

    public: static bool IsActive (void) {return this_!=nullptr;}

    /// Function called in code 
    public: static void ExecuteHook (Hook* hook, Object* o);

    public: static Name GetNodeDisplayName (Node* n);
  };

} // namespace noz


#endif //__noz_Debugger_h__


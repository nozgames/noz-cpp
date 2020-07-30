///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Delegate_h__
#define __noz_Delegate_h__

#include <noz/Reflection/Method.h>

namespace noz {

#if 0
  class Delegate {
    private: ObjectPtr<Object> target_;

    private: Method* method_;

    public: Delegate (Object* target, Method* method);
    public: Delegate (void);

    public: void operator() (noz_int32 argc, Object* argv[]) {
      if(target_ && method_) {
        method_->Invoke(target_,argc,argv);
      }
    }
  };
#endif

} // namespace noz


#endif //__noz_Delegate_h__


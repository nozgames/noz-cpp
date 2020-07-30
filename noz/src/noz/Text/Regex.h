///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Regex_h__
#define __noz_Regex_h__

namespace noz {

  class Regex {
    private: void* handle_;

    public: Regex(void);

    public: Regex(const char* regex);

    public: ~Regex(void);

    public: bool Compile (const char* regex);

    public: bool Execute (const char* value);

    public: String Match (const  char* value);

    public: String Replace (const char* input, const char* replacement);
  };

} // namespace noz


#endif //__noz_Regex_h__


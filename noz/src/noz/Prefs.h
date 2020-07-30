///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Prefs_h__
#define __noz_Prefs_h__

namespace noz {

  class Prefs {
    /// Static class pointer
    private: static Prefs* this_;

    private: std::map<Name,String> prefs_;

    private: String path_;


    public: static void Initialize (void);

    public: static void Uninitialize (void);

    public: static noz_float GetFloat(const Name& name, noz_float def=0.0f);

    public: static noz_int32 GetInt32 (const Name& name, noz_int32 def=0);

    public: static void SetFloat (const Name& name, noz_float value);

    public: static void SetInt32 (const Name& name, noz_int32 value);

    public: static void Save (void);
  };

} // namespace noz


#endif //__noz_Prefs_h__



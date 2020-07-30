///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Environment_h__
#define __noz_Environment_h__

namespace noz {

  enum class SpecialFolder {
    Application,
    ApplicationSupport,
    Document,
    Cache,
    Temp,
  };

  class Environment {

    private: static Environment* this_;

    /// Arguments passed to application
    private: std::vector<String> args_;

    private: String executable_path_;

    private: String executable_dir_;

    private: String executable_name_;

    /// Private constructor
    private: Environment (void);

    /// Static initializer
    public: static void Initialize (int argc, const char* argv[]);

    public: static void Uninitialize (void);

    public: static const std::vector<String>& GetCommandArgs(void) {return this_->args_;}

    /// Return the current working directory 
    public: static String GetCurrentDirectory (void);

    public: static String GetFolderPath(SpecialFolder folder);

    public: static const String& GetExecutablePath (void) {return this_->executable_path_;}

    public: static const String& GetExecutableName (void) {return this_->executable_name_;}

    public: static const String& GetExecutableDirectory (void) {return this_->executable_dir_;}

    public: static void SetCurrentDirectory (const String& dir);
  };

} // namespace noz


#endif //__noz_Prefs_h__



///////////////////////////////////////////////////////////////////////////////
// noZ C-Sharp Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_Directory_h__
#define __noz_IO_Directory_h__

#include "DirectoryInfo.h"

namespace noz {

  class Directory {
    /// Static class..
    private: Directory(void) {}

    /// Creates all directories and subdirectories in the specified path unless they already exist.
    public: static bool CreateDirectory(String path);

    /// Returns an array of all files in the given path
    public: static std::vector<String> GetFiles(String path, bool recursive=false);

    /// Returns the names of subdirectories (including their paths) in the specified directory.
    public: static std::vector<String> GetDirectories(String path, bool recursive=false);

    /// Returns true if the directory with the given path exists.
    public: static bool Exists(const String& path);

    /// Delete the given directory and all of its contents
    public: static bool Delete (const String& path);

    /// Move the given directory 
    public: static bool Move (const String& from, const String& to);

    /// Copy the contents of the directory
    public: static bool Copy (const String& from, const String& to, bool overwrite=false);

    /// Internal recursive create directory implementation
    private: static bool CreateDirectoryInternal(const String& path);
  };

} // namespace noz


#endif // __noz_IO_Directory_h__


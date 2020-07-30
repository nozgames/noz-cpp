///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_File_h__
#define __noz_File_h__

#include "FileStream.h"

namespace noz {

  enum class FileOptions {
    None = 0,
    RandomAccess = 0x10000000,
    DeleteOnClose = 0x04000000,
    SequentialScan = 0x08000000
  };

  class File : public Object {
    NOZ_OBJECT()
    
    /// Static class..
    private: File (void) {}

    /// Returns true if the given file exists.
    public: static bool Exists(const String& path);

    /// Delete the file at the given path
    public: static bool Delete(const String& path);

    /// Copy a file from one location to another
    public: static bool Copy (const String& src, const String& dst, bool overwrite=false);

    /// Move a file from one location to another
    public: static bool Move (const String& src, const String& dst);

    /// Creates or overwrites a file in the specified path.
    public: static FileStream* CreateStream (const String& path, FileMode mode, FileAccess access=FileAccess::ReadWrite);

    public: static DateTime GetLastWriteTime (const String& path);
  };

} // namespace noz


#endif // __noz_File_h__


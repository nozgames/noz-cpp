///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_Path_h__
#define __noz_IO_Path_h__

namespace noz {

  class Path : public Object {
    private: Path(void) {}

    public: static const char PathSeparator;

    /// Returns the extension of the specified path string.
    public: static String GetExtension(String path);

    /// Change the extension of the the given string.
    public: static String ChangeExtension (const String& path, const String& ext);

    /// Returns the directory information for the specified path string.
    public: static String GetDirectoryName(String path);


    public: static String Combine (const String& path1, const String& path2);

    /// Returns the file name and extension of the specified path string.
    public: static String GetFilename (const String& path);

    /// Returns the file name without its extension 
    public: static String GetFilenameWithoutExtension (const String& path);

    /// Returns true if the path begins with a / or a drive letter
    public: static bool IsPathRooted (const String& path);

    /// Returns true if the given character is a valid path separator character for this platform
    public: static bool IsPathSeparator (const char c);

    /// Create a canonical path from the given path.
    public: static String Canonical (const String& path);

    /// Return the full path for the given path
    public: static String GetFullPath (const String& path);

    /// Return the root path
    public: static String GetPathRoot (const String& path);

    /// Return a path that is relative to the given path.  If the path is not 
    /// relative then the returned value will be empty
    public: static String GetRelativePath (const String& path, const String& target);

    /// Returns a unix style path
    public: static String GetUnixPath (const String& path);

    /// Returns a windows style path
    public: static String GetWindowsPath (const String& path);
  };

} // namespace noz


#endif //__noz_IO_Path_h__


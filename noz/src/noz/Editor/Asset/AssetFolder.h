///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AssetFolder_h__
#define __noz_Editor_AssetFolder_h__

namespace noz {
namespace Editor {
  
  class AssetFile;

  class AssetFolder : public Object {
    NOZ_OBJECT()

    friend class AssetDatabase;

    /// Name of the folder
    private: Name name_;

    /// Full path fo the folder
    private: String path_;

    /// Path relative to the base directory used to find the folder
    private: String relative_path_;

    /// Parent folder
    private: AssetFolder* parent_;

    /// List of all sub folders
    private: std::vector<AssetFolder*> folders_;

    /// List of all files within the folder.
    private: std::vector<AssetFile*> files_;

    public: const std::vector<AssetFolder*>& GetFolders(void) const {return folders_;}

    public: const std::vector<AssetFile*>& GetFiles(void) const {return files_;}

    public: bool Contains (AssetFile* file, bool recursive=false) const;

    public: const Name& GetName(void) const {return name_;}

    public: const String& GetPath (void) const {return path_;}

    public: AssetFolder(void);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_AssetFolder_h__


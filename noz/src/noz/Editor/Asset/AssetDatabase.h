///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AssetDatabase_h__
#define __noz_Editor_AssetDatabase_h__

#include "AssetFile.h"
#include "AssetFolder.h"

namespace noz { class DirectoryAssetArchive; }

namespace noz {
namespace Editor {
  
  class AssetEditor;
  class PropertyEditor;

  class AssetDatabase {
    private: struct AssetFileType {
      Name file_ext_;
      Type* file_type_;
      Type* asset_type_;
    };

    public: static AssetRenamedEvent AssetRenamed;

    private: static AssetDatabase* this_;

    /// Path to asset cache
    private: String cache_path_;

    private: DirectoryAssetArchive* cache_archive_;

    private: std::vector<String> asset_paths_;

    private: AssetFolder root_folder_;

    private: std::map<Guid,AssetFile*> files_by_guid_;

    private: std::map<Name,AssetFileType> file_types_by_ext_;

    private: std::map<Type*,AssetFileType*> file_types_by_asset_type_;
    
    private: std::map<Name,AssetFolder*> folders_by_name_;

    private: std::map<Type*, Type*> asset_editors_;

    private: std::map<Type*, Type*> inspector_editors_;

    public: static void Initialize (void);

    public: static void Uninitialize (void);
   
    public: static AssetFolder* CreateFolder (AssetFolder* parent, const char* name);

    public: static AssetFile* CreateFile (AssetFolder* parent, Type* asset_type, const char* name);

    public: static AssetEditor* CreateAssetEditor (AssetFile* file);
    
    public: static bool DeleteFolder (AssetFolder* folder);

    public: static bool DeleteFile (AssetFile* file);

    public: static AssetFile* DuplicateFile (AssetFile* file);

    public: static bool MoveFolder (AssetFolder* folder, const char* to);

    public: static bool MoveFile (AssetFile* file, AssetFolder* folder, const char* name);

    public: static AssetFolder* GetRootFolder (void) {return &this_->root_folder_;}

    public: static Asset* ImportAsset (const Name& name);

    public: static Asset* ImportAsset (const Guid& guid);

    public: static void ReloadAsset (const Guid& guid);

    public: static AssetFile* GetFile (const Guid& guid);

    public: static std::vector<AssetFile*> GetFiles (Type* asset_type, const char* contains);

    private: void Load (void);

    private: void Save (void);

    private: void Refresh (void);

    private: void LoadAssets (void);

    private: void LoadAssets (const String& path);

    private: void LoadAssetFileTypes (void);

    private: void LoadAssetEditors (void);

    private: Guid ReadGuid (const String& path);

    private: void WriteGuid (const String& path, const Guid& guid);

    private: AssetFolder* GetFolder (const String& path, const String& base_path);
  };

} // namespace Edutir
} // namespace noz


#endif // __noz_Editor_AssetDatabase_h__


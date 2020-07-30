///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Nodes/UI/AssetEditor/AssetEditor.h>
#include <noz/Editor/Nodes/UI/PropertyEditor/PropertyEditor.h>
#include <noz/Serialization/BinarySerializer.h>
#include <noz/Assets/DirectoryAssetArchive.h>
#include "AssetDatabase.h"

using namespace noz;
using namespace noz::Editor;


AssetDatabase* AssetDatabase::this_ = nullptr;
AssetRenamedEvent AssetDatabase::AssetRenamed;

static const Name MetaAssetType ("AssetType");
static const Name MetaEditorTarget ("EditorTarget");

static const Name MetaEditorAssetType ("EditorAssetType");
static const Name MetaEditorFileExt ("EditorFileExt");


void AssetDatabase::Initialize(void) {
  if(this_!=nullptr) return;

  // Create a new database instance
  this_ = new AssetDatabase;
  this_->cache_path_ = Path::Combine(Environment::GetFolderPath(SpecialFolder::Cache),"Assets");

  // Add the archive for the cached assets.
  this_->cache_archive_ = new DirectoryAssetArchive(this_->cache_path_);
  AssetManager::AddArchive(this_->cache_archive_);

  // Initialize the root folder
  this_->root_folder_.parent_ = nullptr;
  this_->root_folder_.name_ = "Assets";
  this_->root_folder_.path_ = EditorSettings::GetAssetDirectories()[0];

  this_->folders_by_name_[Name::Empty] = &this_->root_folder_;

  // Find all available asset file types
  this_->LoadAssetFileTypes();
 
  // Find all available editors
  this_->LoadAssetEditors();
  
  // Load all assets from project asset directories
  this_->LoadAssets ();
}

void AssetDatabase::Uninitialize(void) {
  if(this_==nullptr) return;

  delete this_;
  this_ = nullptr;
}

void AssetDatabase::Load(void) {
}

AssetFolder* AssetDatabase::GetFolder (const String& path, const String& base_path) {
  String relative_path = Path::GetRelativePath(path,base_path);

  auto it = folders_by_name_.find(relative_path);

  // If found return it
  if(it != this_->folders_by_name_.end()) return it->second;

  // No parent?
  AssetFolder* parent = &root_folder_;
  if(!Path::GetDirectoryName(relative_path).IsEmpty()) {
    parent = GetFolder(Path::GetDirectoryName(path),base_path);
    noz_assert(parent);
  }
  
  // Create a new folder.
  AssetFolder* folder = new AssetFolder;
  folder->name_ = Path::GetFilename(path);
  folder->path_ = path;
  folder->parent_ = parent;
  folder->relative_path_ = relative_path;
  this_->folders_by_name_[relative_path] = folder;

  // Add to the parents folder list
  parent->folders_.push_back(folder);

  return folder;    
}

void AssetDatabase::LoadAssetEditors (void) {
  // Iterate over all known types and find any that are a subclass of AssetEditor
  const std::vector<Type*>& types = Type::GetTypes();
  for(auto it=types.begin(); it!=types.end(); it++) {
    Type* t = *it;
    noz_assert(t);

    // Skip if not a subclass of AssetEditor
    if(!t->IsCastableTo(typeof(AssetEditor))) continue;

    // Skip the type if it has no allocator
    if(!t->HasAllocator()) continue;

    // Retrieve the importer extension name    
    Name asset_type_name = t->GetMeta(MetaAssetType);
    if(asset_type_name.IsEmpty()) {
      Console::WriteLine("%s: error: missing 'AssetType' attribute on AssetEditor'", t->GetQualifiedName().ToCString());
      continue;
    }

    Type* asset_type = Type::FindType(asset_type_name);
    if(nullptr==asset_type) {
      Console::WriteLine("%s: error: unknown AssetType '%s' on AssetEditor'", t->GetQualifiedName().ToCString(),asset_type_name.ToCString());
      continue;
    }

    asset_editors_[asset_type] = t;
  }
}

void AssetDatabase::LoadAssetFileTypes (void) {
  // Iterate over all known types and find any that are a subclass of AssetImporter
  const std::vector<Type*>& types = Type::GetTypes();
  for(auto it=types.begin(); it!=types.end(); it++) {
    Type* t = *it;
    noz_assert(t);

    // Skip if not asubclass of AssetFile or has no allocator
    if(!t->IsCastableTo(typeof(AssetFile))) continue;
    if(!t->HasAllocator()) continue;

    // File extension..
    Name mext = t->GetMeta(MetaEditorFileExt);
    if(mext.IsEmpty()) {
      Console::WriteLine("%s: error: missing 'EditorFileExt' attribute",t->GetQualifiedName().ToCString());
      continue;
    }

    // File asset type..
    Name asset_type_name = t->GetMeta(MetaEditorAssetType);
    if(asset_type_name.IsEmpty()) {
      Console::WriteLine("%s: error: missing 'EditorFileType' attribute",t->GetQualifiedName().ToCString());
      continue;
    }

    Type* asset_type = Type::FindType(asset_type_name);
    if(nullptr == asset_type) {
      Console::WriteLine("%s: error: unknown type '%s' for 'EditorAssetType' attribute",t->GetQualifiedName().ToCString(),asset_type_name.ToCString());
      continue;
    }

    // Ensure the asset type is actually an asset
    if(!asset_type->IsCastableTo(typeof(Asset))) {
      Console::WriteLine("%s: error: asset type '%s' for 'EditorAssetType' attribute is not an Asset",t->GetQualifiedName().ToCString(),asset_type_name.ToCString());
      continue;
    }

    // Add the type to the map
    AssetFileType& r = file_types_by_ext_[mext];
    r.asset_type_ = asset_type;
    r.file_type_ = t;
    r.file_ext_ = mext;

    file_types_by_asset_type_[asset_type] = &r;
  }  
}

void AssetDatabase::LoadAssets (void) {
  for(auto it=EditorSettings::GetAssetDirectories().begin(); it!=EditorSettings::GetAssetDirectories().end(); it++) {
    LoadAssets(*it);
  }
}

void AssetDatabase::LoadAssets (const String& assets_path) {
  asset_paths_.push_back(assets_path);  

  std::vector<String> dirs = Directory::GetDirectories(assets_path,true);
  for(auto it=dirs.begin(); it!=dirs.end(); it++) {
    GetFolder(Path::Canonical(*it), assets_path);
  }

  String app_path = Environment::GetFolderPath(SpecialFolder::Application);
  String base_path = Path::Canonical(Path::Combine(app_path, assets_path));

  // Return full list of all files within the assets directory.
  std::vector<String> files = Directory::GetFiles(Path::Combine(app_path,assets_path), true);  
  for(auto it=files.begin(); it!=files.end(); it++) {
    String path = (*it).Substring(app_path.GetLength()+1);
    String ext = Path::GetExtension(path);
    String dir = Path::GetDirectoryName(path);
    
    // Ensure the file has an extension to identify it by
    if(ext.IsEmpty()) continue;

    // Trim off the dot.
    ext = ext.Substring(1);

    // Find asset file type that matches the file
    auto itft = file_types_by_ext_.find(ext);
    if(itft == file_types_by_ext_.end()) continue;

    // Create the file
    AssetFile* file = itft->second.file_type_->CreateInstance<AssetFile>();
    if(nullptr==file) continue;

    // Strip extension to get asset name.
    String asset_name = Path::ChangeExtension(path,"");  

    // Rad the guid for this asset if there is one.
    Guid guid = ReadGuid ((*it));

    // If there is no guid..
    if(guid.IsEmpty()) {
      // Generate a new guid
      guid = Guid::Generate();

      // Write the guid to associate it with the asset
      WriteGuid(*it,guid);
    }

    // Asset to ensure there are no duplicate guid's in the file system
    NOZ_TODO("Handle this by converting a guid?");
    noz_assert(files_by_guid_.find(guid) == files_by_guid_.end());

    files_by_guid_[guid] = file;
    file->asset_type_ = itft->second.asset_type_;
    file->ext_ = ext;
    file->asset_name_ = asset_name;
    file->guid_ = guid;
    file->path_ = Path::Canonical(*it);
    file->name_ = Path::GetFilenameWithoutExtension(file->path_);
    file->folder_ = GetFolder(Path::GetDirectoryName(Path::Canonical(*it)), base_path);
    file->folder_->files_.push_back(file);

    String cache_path = this_->cache_archive_->GetFileName(file->guid_);

    DateTime asset_modified = file->GetModifiedDate();
    DateTime cache_modified = File::GetLastWriteTime(cache_path);
        
    // If the asset was modified since it was last cached then delete the cache file
    // so it will be regenerated the next time it is loaded
    if(!cache_modified.IsEmpty() && asset_modified > cache_modified) {
      File::Delete(cache_path);
    }
  }
}

Guid AssetDatabase::ReadGuid (const String& path) {
  String guid_path = String::Format("%s.nozguid", path.ToCString());
  FileStream fs;
  if(!fs.Open(guid_path,FileMode::Open)) return Guid::Empty;
  
  noz_uint32 size = fs.GetLength();
  char* buffer = new char[size+1];
  fs.Read(buffer,0,size);
  buffer[size] = 0;
  fs.Close();

  return Guid::Parse(buffer); 
}

void AssetDatabase::WriteGuid (const String& path, const Guid& guid) {
  String guid_path = String::Format("%s.nozguid", path.ToCString());
  String guid_str = String::Format("{%s}", guid.ToString().ToCString() );

  FileStream fs;
  if(!fs.Open(guid_path,FileMode::Truncate)) return;
  fs.Write((char*)guid_str.ToCString(), 0, guid_str.GetLength());
  fs.Close();
}

Asset* AssetDatabase::ImportAsset (const Name& name) {
  return nullptr;
}

Asset* AssetDatabase::ImportAsset (const Guid& guid) {
  if(nullptr==this_) return nullptr;

  auto it = this_->files_by_guid_.find(guid);
  if(it==this_->files_by_guid_.end()) return nullptr;

  AssetFile& file = *it->second;
  Asset* asset = file.Import();
  Console::WriteLine("imported: %s", file.path_.ToCString());
  if(nullptr == asset) return nullptr;

  String cache_filename = this_->cache_archive_->GetFileName(file.guid_);

  FileStream fs;
  if(!fs.Open(cache_filename, FileMode::Truncate)) {
    return asset;
  }

  BinarySerializer().Serialize(asset,&fs);
  fs.Close();

  return asset;
}


AssetEditor* AssetDatabase::CreateAssetEditor (AssetFile* file) {
  noz_assert(this_);
  noz_assert(file);

  auto it = this_->asset_editors_.find(file->asset_type_);
  if(it == this_->asset_editors_.end()) {
    Console::WriteLine("%s: error: no associated editor available for asset type '%s'", file->GetPath().ToCString(), file->asset_type_->GetQualifiedName().ToCString());
    return nullptr;
  }

  AssetEditor* editor = it->second->CreateInstance<AssetEditor>();
  if(nullptr == editor) {
    Console::WriteLine("%s: error: failed to create editor of type '%s' for asset type '%s'", file->GetPath().ToCString(), it->second->GetQualifiedName().ToCString(),it->first->GetQualifiedName().ToCString());
    return nullptr;
  }

  return editor;
}

AssetFile* AssetDatabase::GetFile (const Guid& guid) {
  auto it = this_->files_by_guid_.find(guid);
  if(it==this_->files_by_guid_.end()) return nullptr;
  return it->second;
}

std::vector<AssetFile*> AssetDatabase::GetFiles (Type* asset_type, const char* contains) {
  std::vector<AssetFile*> result;
  for(auto it=this_->files_by_guid_.begin(); it!=this_->files_by_guid_.end(); it++) {
    AssetFile* file = it->second;

    // Ensure the type matches.
    if(!file->asset_type_->IsCastableTo(asset_type)) continue;
    
    // Now check for the name containing the given text
    if(contains && -1==file->name_.ToString().IndexOf(contains,0,StringComparison::OrdinalIgnoreCase)) continue;

    result.push_back(file);
  }

  return result;
}

void AssetDatabase::ReloadAsset (const Guid& guid) {
  if(nullptr==this_) return;
  if(!AssetManager::IsAssetLoaded(guid)) return;

  auto it = this_->files_by_guid_.find(guid);
  if(it==this_->files_by_guid_.end()) return;

  AssetFile& file = *it->second;

  // Get a pointer to the actual asset.
  Asset* asset = AssetManager::LoadAsset(typeof(Asset), guid);
  if(nullptr==asset) return;

  if(!file.Reimport(asset)) {
    return;
  }

  // Iterate over all known objects and set new asset
  for(noz_uint32 i=1,c=ObjectManager::GetObjectCount(); i<c; i++) {
    Object* o = ObjectManager::GetObject(i);
    if(o==nullptr) continue;

    for(Type* t=o->GetType(); t; t=t->GetBase()) {
      for(auto itp=t->GetProperties().begin(); itp!=t->GetProperties().end(); itp++) {
        Property* p = *itp;
        if(p->IsTypeOf(typeof(ObjectPtrProperty))) {
          ObjectPtrProperty* opp = (ObjectPtrProperty*)p;
          if(opp->Get(o) == asset) {
            opp->Set(o,nullptr);
            opp->Set(o,asset);
          }
        }
      }
    }
  }
}

AssetFolder* AssetDatabase::CreateFolder (AssetFolder* parent, const char* name) {
  noz_assert(parent);
  noz_assert(name);
  noz_assert(*name);

  String actual_name = name;
  String path = Path::Combine(parent->GetPath(),actual_name);
  if(Directory::Exists(path)) {
    for(noz_uint32 i=2; ; i++) {
      actual_name = String::Format ("%s %d", name, i);
      path = Path::Combine(parent->GetPath(),actual_name);
      if(!Directory::Exists(path)) {
        break;
      }
    }
  }

  if(!Directory::CreateDirectory(path)) {
    return nullptr;
  }
  

  // Create a new folder.
  AssetFolder* folder = new AssetFolder;
  folder->name_ = actual_name;
  folder->path_ = path;
  folder->parent_ = parent;
  folder->relative_path_ = Path::Combine(parent->relative_path_, actual_name);
  this_->folders_by_name_[folder->relative_path_] = folder;

  // Add to the parents folder list
  parent->folders_.push_back(folder);

  return folder;
}

AssetFile* AssetDatabase::CreateFile (AssetFolder* parent, Type* asset_type, const char* name) {
  noz_assert(parent);
  noz_assert(asset_type);
  noz_assert(asset_type->IsAsset());

  // Find the file type information from the asset type
  auto itft=this_->file_types_by_asset_type_.find(asset_type);
  if(itft==this_->file_types_by_asset_type_.end()) return nullptr;

  // Create an asset file for the type
  AssetFile* file = itft->second->file_type_->CreateInstance<AssetFile>();
  if(nullptr == file) return nullptr;

  String actual_name = name;
  String actual_filename = String::Format("%s.%s", name, itft->second->file_ext_.ToCString());
  String actual_path = Path::Combine(parent->GetPath(), actual_filename);
  if(File::Exists(actual_path)) {
    for(noz_uint32 i=2; ; i++) {
      actual_name = String::Format("%s%d", name, i);
      actual_filename = String::Format ("%s.%s", actual_filename.ToCString(), itft->second->file_ext_.ToCString());
      actual_path = Path::Combine(parent->GetPath(),actual_filename);
      if(!File::Exists(actual_path)) break;
    }
  }
  
  // populate the file
  file->ext_ = itft->second->file_ext_;
  file->asset_type_ = itft->second->asset_type_;
  file->asset_name_ = Path::Combine(parent->relative_path_, actual_name);
  file->folder_ = parent;
  file->name_ = actual_name;
  file->path_ = actual_path;
  file->guid_ = Guid::Generate();
  if(!file->CreateEmpty()) {
    delete file;
    return nullptr;
  }

  file->folder_->files_.push_back(file);
  this_->files_by_guid_[file->guid_] = file;       

  this_->WriteGuid(actual_path,file->guid_);

  return file;
}

bool AssetDatabase::DeleteFolder (AssetFolder* folder) {
  // Delete the folder..
  if(!Directory::Delete(folder->path_)) return false;

  // Remove from parent folder list.
  if(folder->parent_) {
    for(noz_uint32 i=0,c=folder->parent_->folders_.size(); i<c; i++) {
      if(folder->parent_->folders_[i] == folder) {
        folder->parent_->folders_.erase(folder->parent_->folders_.begin()+i);
        break;
      }
    }
  }

  // Remove from global list.
  auto it = this_->folders_by_name_.find(folder->name_);
  if(it != this_->folders_by_name_.end()) {
    this_->folders_by_name_.erase(it);
  }

  delete folder;

  return true;
}

bool AssetDatabase::DeleteFile (AssetFile* file) {
  // Delete the main file.
  if(!File::Delete(file->path_)) return false;

  // Delete the guid file.
  File::Delete(String::Format("%s.nozguid", file->path_.ToCString()));

  // Give derived classes a chance to handle the deletion.
  file->OnDeleted ();

  // Remove from parent folder list.
  for(noz_uint32 i=0,c=file->folder_->files_.size(); i<c; i++) {
    if(file->folder_->files_[i] == file) {
      file->folder_->files_.erase(file->folder_->files_.begin()+i);
      break;
    }
  }

  // Remove from the global list.
  auto it = this_->files_by_guid_.find(file->guid_);
  if(it != this_->files_by_guid_.end()) {
    this_->files_by_guid_.erase(it);
  }

  // Delete the object
  delete file;

  return true;
}

bool AssetDatabase::MoveFolder (AssetFolder* folder, const char* to) {
  noz_assert(folder);
  noz_assert(to);
  noz_assert(*to);

  String target = Path::Combine(Path::GetDirectoryName(folder->path_), to);
  if(Directory::Exists(target) || File::Exists(target)) {
    return false;
  }

  if(!Directory::Move(folder->path_, target)) {
    return false;
  }

  folder->path_ = target;
  folder->name_ = to;
  folder->relative_path_ = Path::Combine(Path::GetDirectoryName(folder->relative_path_),to);

  return true;
}

bool AssetDatabase::MoveFile (AssetFile* file, AssetFolder* target_folder, const char* target_name) {
  noz_assert(file);
  noz_assert(target_folder);
  noz_assert(target_name);
  noz_assert(*target_name);

  String source_path = file->GetPath();
  String target_path = Path::Combine(target_folder->GetPath(),String::Format("%s.%s",target_name,file->GetExtension().ToCString()));

  if(!File::Move(source_path, target_path)) {
    return false;
  }

  file->path_ = target_path;
  file->name_ = target_name;
  file->asset_name_ = Path::Combine(target_folder->relative_path_,target_name);

  // Remove from parent folder list.
  for(noz_uint32 i=0,c=file->folder_->files_.size(); i<c; i++) {
    if(file->folder_->files_[i] == file) {
      file->folder_->files_.erase(file->folder_->files_.begin()+i);
      break;
    }
  }

  file->folder_ = target_folder;
  target_folder->files_.push_back(file);

  // Give derived classes a change to handle the move
  file->OnMoved (source_path, target_path);

  // Move the guid file and it that fails just make a new one.
  if(!File::Move(String::Format("%s.nozguid", source_path.ToCString()), String::Format("%s.nozguid", target_path.ToCString()))) {
    this_->WriteGuid(file->path_, file->guid_);
  }

  AssetDatabase::AssetRenamed(file);

  return true;
}

AssetFile* AssetDatabase::DuplicateFile (AssetFile* file) {
  noz_assert(file);

  // Find a unique filename for the duplicated file.
  String target_path;
  String target_name;
  for(noz_int32 i=1; ; i++) {
    target_name = String::Format("%s %d", file->name_.ToCString(), i);
    String test_path = Path::Combine(file->folder_->path_,String::Format("%s.%s", target_name.ToCString(), file->ext_.ToCString()));
    if(!File::Exists(test_path)) {
      target_path = test_path;
      break;
    }
  }

  // Create an asset file for the type
  AssetFile* duplicate = file->GetType()->CreateInstance<AssetFile>();
  if(nullptr == duplicate) return nullptr;

  // Copy the file
  if(!File::Copy(file->path_, target_path)) {
    delete duplicate;
    return nullptr;
  }

  // Initialzie the duplicated file.
  duplicate->ext_ = file->ext_;
  duplicate->asset_type_ = file->asset_type_;
  duplicate->asset_name_ = Path::Combine(file->folder_->relative_path_, target_name);
  duplicate->folder_ = file->folder_;
  duplicate->name_ = target_name;
  duplicate->path_ = target_path;
  duplicate->guid_ = Guid::Generate();
  duplicate->folder_->files_.push_back(duplicate);
  this_->files_by_guid_[duplicate->guid_] = duplicate;

  // Write the guid file 
  this_->WriteGuid(target_path,duplicate->guid_);

  return duplicate;
}

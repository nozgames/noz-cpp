///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AssetManager.h"
#include <noz/IO/BinaryReader.h>
#include <noz/Serialization/BinaryDeserializer.h>

#if defined(NOZ_EDITOR)
#include <noz/Editor/Asset/AssetDatabase.h>
#endif

using namespace noz;

AssetManager* AssetManager::instance_ = nullptr;


void AssetManager::Initialize(void) {
  if(nullptr != instance_) {
    return;
  }

  instance_ = new AssetManager;
}

void AssetManager::Uninitialize(void) {
  delete instance_;
  instance_= nullptr;
}


AssetManager::AssetManager(void) {
}

AssetManager::~AssetManager(void) {
  // Clean up any registered archives.
  for(auto it=archives_.begin(); it!=archives_.end(); it++) {
    delete (*it);
  }
  archives_.clear();
}

void AssetManager::AddAsset (const Guid& guid, Asset* asset) {
  noz_assert(asset);
  noz_assert(!guid.IsEmpty());

  asset->managed_ = true;
  asset->guid_ = guid;
  instance_->assets_[guid] = (Asset*)asset;
}

Asset* AssetManager::LoadAsset(Type* type, const Guid& guid) {
  if(instance_==nullptr) return nullptr;

  // Is the asset already loaded?
//  if(type->IsManagedAsset()) {
    auto it = instance_->assets_.find(guid);
    if(it != instance_->assets_.end() && it->second != nullptr) {
      // Validate the asset type
      if(!it->second->IsTypeOf(type)) return nullptr;

      // Return the asset
      return (Asset*)it->second;
    }
//  }

  Asset* asset = nullptr;
  for(auto itar=instance_->archives_.rbegin(); asset==nullptr && itar!=instance_->archives_.rend(); itar++) {
    AssetArchive* a = *itar;
    noz_assert(a);
    
    Stream* file = a->OpenFile(guid);
    if(file != nullptr) {
      Object* o = BinaryDeserializer().Deserialize(file);
      if(o) {
        if(o->GetType()->IsAsset()) {          
          asset = (Asset*)o;
        } else {
          delete o;
        }
      }
      delete file;
    }    
  }

  // If no asset was found then we are done
  if(nullptr == asset) {
#if defined(NOZ_EDITOR)
    asset = Editor::AssetDatabase::ImportAsset(guid);
    if(nullptr==asset) return nullptr;
#else
    return nullptr;
#endif
  }

  //asset->name_ = name;
  asset->guid_ = guid;

  // manage the asset if needed
  if(type->IsManagedAsset()) {
    asset->managed_ = true;
    instance_->assets_[guid] = asset;
  }

  // Allow the asset to handle being loaded.
  asset->OnLoaded();

  // Validate the asset type
  if(!asset->IsTypeOf(type)) return nullptr;

  // Return the asset
  return asset;
}

Asset* AssetManager::LoadAsset(Type* type, const Name& name) {
#if 0
  if(instance_==nullptr) return nullptr;

  // Is the asset already managed?
  if(type->IsManagedAsset()) {
    auto it = instance_->assets_.find(name);
    if(it != instance_->assets_.end() && it->second != nullptr) {
      // Validate the asset type
      if(!it->second->IsTypeOf(type)) {
        return nullptr;
      }

      // Return the asset
      return (Asset*)it->second;
    }
  }

  String filename = Path::ChangeExtension(name,".nozasset");

  Asset* asset = nullptr;
  for(auto itar=instance_->archives_.rbegin(); asset==nullptr && itar!=instance_->archives_.rend(); itar++) {
    AssetArchive* a = *itar;
    noz_assert(a);
    
    Stream* file = a->OpenFile(filename);
    if(file != nullptr) {
      Object* o = BinaryDeserializer().Deserialize(file);
      if(o) {
        if(o->GetType()->IsAsset()) {          
          asset = (Asset*)o;
        } else {
          delete o;
        }
      }
      delete file;
    }    
  }

  // If no asset was found then we are done
  if(nullptr == asset) {
    return nullptr;
  }

  asset->name_ = name;

  // manage the asset if needed
  if(type->IsManagedAsset()) {
    asset->managed_ = true;
    instance_->assets_[name] = asset;
  }

  // Allow the asset to handle being loaded.
  asset->OnLoaded();

  return asset;
#else
  return nullptr;
#endif
}

void AssetManager::AddArchive(AssetArchive* archive, bool override) {
  if(override || instance_->archives_.empty()) {
    instance_->archives_.push_back(archive);
  } else {
    instance_->archives_.insert(instance_->archives_.begin(), archive);
  }
}

void AssetManager::RemoveArchive (AssetArchive* archive) {
  if(nullptr == instance_) return;

  noz_assert(archive);
  for(noz_uint32 i=0,c=instance_->archives_.size(); i<c; i++) {
    if(instance_->archives_[i] == archive) {
      instance_->archives_.erase(instance_->archives_.begin() + i);
      return;
    }
  }
}

bool AssetManager::IsAssetLoaded (const Guid& guid) {
  return instance_->assets_.find(guid) != instance_->assets_.end();
}


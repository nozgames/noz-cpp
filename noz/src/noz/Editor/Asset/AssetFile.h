///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AssetFile_h__
#define __noz_Editor_AssetFile_h__

namespace noz {
namespace Editor {

  class AssetDatabase;
  class AssetFolder;
  class AssetEditor;

  class AssetFile : public Object {
    NOZ_OBJECT(Abstract)

    friend class AssetDatabase;

    private: Name name_;

    private: Name asset_name_;

    private: Guid guid_;

    /// Full path to the asset file.
    private: String path_;

    /// Extension of file.
    private: Name ext_;

    /// Thumbnail
    private: ObjectPtr<Image> thumbnail_;

    /// Pointer to the actual asset 
    private: ObjectPtr<Asset> asset_;

    /// Folder the asset resides in.
    private: AssetFolder* folder_;

    private: Type* asset_type_;

    public: AssetFile(void);

    /// Return the globally unique identifier of the file.
    public: const Guid& GetGuid (void) const {return guid_;}

    /// Return the full path of the asset file
    public: const String& GetPath(void) const {return path_;}

    /// Return the name of the asset
    public: const Name& GetName (void) const {return name_;}

    /// Return the asset name
    public: const Name& GetAssetName (void) const {return asset_name_;}

    /// Return the asset file extension
    public: const Name& GetExtension(void) const {return ext_;}

    /// Return the folder the file is contained in
    public: AssetFolder* GetFolder(void) const {return folder_;}

    public: Type* GetAssetType(void) const {return asset_type_;}

    public: AssetEditor* CreateEditor (void) const;

    public: virtual bool CreateEmpty (void) {return false;}

    public: virtual Asset* Import (void) {return nullptr;}

    public: virtual bool Reimport (Asset* asset) {return false;}


    /// Called when the file is moved
    protected: virtual void OnMoved (const String& from, const String& to) { }

    /// Called when the file is deleted.
    protected: virtual void OnDeleted (void) { }

    /// Called when the file is duplicated
    protected: virtual void OnDuplicated (const String& to) { }

    public: virtual bool CanCreateNode (void) const {return false;}

    public: virtual Node* CreateNode (void) const {return nullptr;};

    public: virtual DateTime GetModifiedDate (void) const;
  };


} // namespace Editor
} // namespace noz


#endif // __noz_Editor_AssetFile_h__


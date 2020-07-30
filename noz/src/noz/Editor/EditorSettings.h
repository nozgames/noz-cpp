///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_EditorSettings_h__
#define __noz_Editor_EditorSettings_h__

namespace noz {
 
  class Resolution : public Object {
    NOZ_OBJECT()

    private: NOZ_PROPERTY(Name=Width) noz_uint32 width_;
    private: NOZ_PROPERTY(Name=Height) noz_uint32 height_;
    private: NOZ_PROPERTY(Name=Name) Name name_;

    public: Resolution (void) : width_(0), height_(0) {}
    public: Resolution (const Name& n, noz_uint32 w, noz_uint32 h) : width_(w), height_(h), name_(n) {}

    public: noz_uint32 GetWidth (void) const {return width_;}
    public: noz_uint32 GetHeight (void) const {return height_;}
    public: const Name& GetName (void) const {return name_;}
  };

  class EditorSettings : public Object {    
    NOZ_OBJECT()

    private: NOZ_PROPERTY(Name=EditorScene) Guid editor_scene_guid_;
    private: NOZ_PROPERTY(Name=TestPrefab) Guid test_prefab_guid_;
    private: NOZ_PROPERTY(Name=MakefilePath) String makefile_path_;
    private: NOZ_PROPERTY(Name=AssetDirectories) std::vector<String> asset_directories_;
    private: NOZ_PROPERTY(Name=Resolutions) std::vector<ObjectPtr<Resolution>> resolutions_;

    private: static EditorSettings* this_;

    protected: EditorSettings (void);

    public: static void Initialize (void);

    public: static void Uninitialize (void);

    public: static const Guid& GetEditorSceneGuid (void) {return this_->editor_scene_guid_;}

    public: static const Guid& GetTestPrefabGuid (void) {return this_->test_prefab_guid_;}

    public: static const String& GetMakefilePath (void) {return this_->makefile_path_;}

    public: static const std::vector<String>& GetAssetDirectories (void) {return this_->asset_directories_;}

    public: static noz_uint32 GetResolutionCount (void) {return this_->resolutions_.size();}

    public: static const Resolution& GetResolution (noz_uint32 i) {return *this_->resolutions_[i];}

    public: static void AddResolution (const Name& name, noz_uint32 width, noz_uint32 height) {
      this_->resolutions_.push_back(new Resolution(name,width,height));
    }
  };

} // namespace noz


#endif // __noz_Editor_EditorSettings_h__

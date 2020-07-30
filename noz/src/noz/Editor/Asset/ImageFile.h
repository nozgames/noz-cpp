///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_ImageFile_h__
#define __noz_Editor_ImageFile_h__

#include <noz/Render/Imaging/ImageFilter.h>

namespace noz {
namespace Editor {

  class ImageDef : public Object {
    NOZ_OBJECT()

    public: NOZ_PROPERTY(Name=WrapMode) ImageWrapMode wrap_mode_;
    public: NOZ_PROPERTY(Name=FilterMode) ImageFilterMode filter_mode_;
    public: NOZ_PROPERTY(Name=SDF) bool sdf_;
    public: NOZ_PROPERTY(Name=ResizeWidth) noz_int32 resize_width_;
    public: NOZ_PROPERTY(Name=ResizeHeight) noz_int32 resize_height_;

    ImageDef(void);

    ~ImageDef(void);
  };

  class ImageFile : public AssetFile {
    NOZ_OBJECT(Abstract)

    public: virtual Asset* Import (void) override;

    public: virtual bool Reimport (Asset* asset) override;

    private: Image* Import (Stream* stream, ImageDef& def);
    
    public: virtual bool CanCreateNode (void) const override {return true;}

    public: virtual Node* CreateNode (void) const override;

    public: virtual DateTime GetModifiedDate (void) const override;
  };

  class PNGImageFile : public ImageFile {
    NOZ_OBJECT(EditorFileExt=png, EditorAssetType=noz::Image)
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_ImageFile_h__


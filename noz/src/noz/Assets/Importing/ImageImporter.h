///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Importers_ImageImporter_h__
#define __noz_Importers_ImageImporter_h__

#include <noz/Assets/Importing/AssetImporter.h>
#include <noz/Render/Imaging/ImageFilter.h>

namespace noz {

  class ImageMeta : public Object {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Filters)
    public: std::vector<noz::ImageFilter*> filters_;

    ImageMeta(void);

    ~ImageMeta(void);
  };

  class ImageImporter : public AssetImporter {
    NOZ_OBJECT()

    public: virtual Asset* Import (const String& path) override;

    private: Image* Import (Stream* stream);
  };

  class PNGImporter : public ImageImporter {
    NOZ_OBJECT(ImportEXT=png, ImportType=Image)
  };

} // namespace noz

#endif // __noz_Importers_ImageImporter_h__


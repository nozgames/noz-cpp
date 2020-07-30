///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Importers_PNGImporter_h__
#define __noz_Importers_PNGImporter_h__

#include "ImageImporter.h"

namespace noz {

  class PNGImporterOld : public AssetImporter {
    NOZ_OBJECT(ImportEXT=oldpng, ImportType=Image)

    public: virtual Asset* Import (const String& path) override;

    private: Image* Import (Stream* stream);
  };

} // namespace noz

#endif // __noz_Importers_ImageImporter_h__


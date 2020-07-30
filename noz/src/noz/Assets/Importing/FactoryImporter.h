///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_FactoryImporter_h__
#define __noz_FactoryImporter_h__

#include <noz/Assets/Importing/AssetImporter.h>

namespace noz {

  class FactoryImporter : public AssetImporter {
    NOZ_OBJECT(ImportEXT=nozfactory,ImportType=Factory)

    public: virtual Asset* Import (const String& path) override;
  };

} // namespace noz

#endif // __noz_ControlTemplateImporter_h__


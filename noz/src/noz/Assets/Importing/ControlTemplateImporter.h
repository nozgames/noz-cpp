///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ControlTemplateImporter_h__
#define __noz_ControlTemplateImporter_h__

#include <noz/Assets/Importing/AssetImporter.h>

namespace noz {

  class ControlTemplateImporter : public AssetImporter {
    NOZ_OBJECT(ImportEXT=nozcontrol,ImportType=ControlTemplate)

    public: virtual Asset* Import (const String& path) override;
  };

} // namespace noz

#endif // __noz_ControlTemplateImporter_h__


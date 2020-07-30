///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Importers_TextImporter_h__
#define __noz_Importers_TextImporter_h__

#include <noz/Assets/Importing/AssetImporter.h>

namespace noz {

  class TextImporter : public AssetImporter {
    NOZ_OBJECT()

    private: enum TextType {
      Unknown,
      Json
    };

    public: virtual Asset* Import (const String& path) override;

    private: TextType GetTextType (Stream* stream);
  };

} // namespace noz

#endif // __noz_Importers_TextImporter_h__


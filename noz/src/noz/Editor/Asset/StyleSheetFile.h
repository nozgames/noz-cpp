///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_StyleSheetFile_h__
#define __noz_Editor_StyleSheetFile_h__

#include "AssetFile.h"

namespace noz {
namespace Editor {

  class StyleSheetFile : public AssetFile {
    NOZ_OBJECT(EditorFileExt=nozstylesheet,EditorAssetType=noz::StyleSheet)

    public: virtual bool CreateEmpty (void) override;

    public: virtual Asset* Import (void) override;
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_StyleSheetFile_h__


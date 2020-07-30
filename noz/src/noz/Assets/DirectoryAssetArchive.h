///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DirectoryAssetArchive_h__
#define __noz_DirectoryAssetArchive_h__

#include "AssetArchive.h"

namespace noz {

  class DirectoryAssetArchive : public AssetArchive {
    NOZ_OBJECT()

    private: String path_;

    public: DirectoryAssetArchive(const String& path);

    public: virtual Stream* OpenFile (const Guid& guid) override;

    public: String GetFileName (const Guid& guid) const;
  };

} // namespace noz


#endif // __noz_DirectoryAssetArchive_h__


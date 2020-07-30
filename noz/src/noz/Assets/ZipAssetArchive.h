///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ZipAssetAarchive_h__
#define __noz_ZipAssetAarchive_h__

#include "AssetArchive.h"

namespace noz {

  class ZipAssetArchive : public AssetArchive {
    NOZ_OBJECT()

    public: virtual Stream* OpenFile (const Guid& guid) override {return nullptr;}
  };

} // namespace noz


#endif // __noz_ZipAssetAarchive_h__


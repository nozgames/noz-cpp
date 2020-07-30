///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AssetArchive_h__
#define __noz_AssetArchive_h__

#include <noz/IO/Stream.h>
#include <noz/IO/Path.h>

namespace noz {
 
  class AssetArchive : public Object {
    NOZ_OBJECT()

    public: virtual Stream* OpenFile (const Guid& guid) = 0;
  };

} // namespace noz


#endif //__noz_AssetArchive_h__


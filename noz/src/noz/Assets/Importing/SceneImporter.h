///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SceneImporter_h__
#define __noz_SceneImporter_h__

#include "TextImporter.h"

namespace noz {

  class SceneImporter : public TextImporter {
    NOZ_OBJECT(ImportEXT=nozscene,ImportType=Scene)
  };

} // namespace noz

#endif // __noz_SceneImporter_h__


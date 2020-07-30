///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SpriteImporter_h__
#define __noz_SpriteImporter_h__

#include "TextImporter.h"

namespace noz {

  class SpriteImporter : public TextImporter {
    NOZ_OBJECT(ImportEXT=nozsprite,ImportType=Sprite)
  };

} // namespace noz

#endif // __noz_SpriteImporter_h__


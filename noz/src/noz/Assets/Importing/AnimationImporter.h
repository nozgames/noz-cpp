///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AnimationImporter_h__
#define __noz_AnimationImporter_h__

#include "TextImporter.h"

namespace noz {

  class AnimationImporter : public TextImporter {
    NOZ_OBJECT(ImportEXT=nozanim,ImportType=Animation)    
  };

} // namespace noz

#endif // __noz_AnimationImporter_h__


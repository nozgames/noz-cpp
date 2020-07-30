///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SceneNode_h__
#define __noz_SceneNode_h__

#include <noz/Viewport.h>

namespace noz {

  class SceneNode : public Viewport {
    NOZ_OBJECT(EditorIcon="{052EC1DA-EE1E-4552-A11C-C64A74CE4059}",EditorName=Scene)

    friend class Scene;

    public: SceneNode(void);

    public: ~SceneNode(void);
  };

} // namespace noz


#endif // __noz_SceneNode_h__


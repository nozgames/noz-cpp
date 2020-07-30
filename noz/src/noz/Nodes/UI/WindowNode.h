///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_WindowNode_h__
#define __noz_WindowNode_h__

#include "UINode.h"

namespace noz {

  class WindowNode : public UINode {
    NOZ_OBJECT() 

    friend class Window;
    friend class Application;

    public: WindowNode (Window* window);

    public: ~WindowNode (void);

    protected: virtual void Render (RenderContext* rc) override;
    protected: virtual void Arrange (const Rect& r) override;
    protected: virtual Vector2 Measure (const Vector2& a) override;
  };

} // namespace noz


#endif //__noz_WindowNode_h__



///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RemoteClientRenderer_h__
#define __noz_RemoteClientRenderer_h__

#include "Renderer.h"

namespace noz {

  class RemoteClientRenderer : public Renderer {
    NOZ_OBJECT()

    public: RemoteClientRenderer (void);

    /// Called before rendering of the node begins
    public: virtual void RenderBegin(RenderContext* rc) override;

    /// Called to render the content.
    NOZ_FIXME()
    //public: virtual void Render(RenderContext* rc) override;

    /// Called after the node and all of its children have been rendered.
    public: virtual void RenderEnd(RenderContext* rc) override;
  };

} // namespace noz


#endif //__noz_RemoteClientRenderer_h__



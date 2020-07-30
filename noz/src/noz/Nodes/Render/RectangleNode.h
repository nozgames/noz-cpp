///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RectangleNode_h__
#define __noz_RectangleNode_h__

#include "RenderNode.h"

namespace noz {

  class RectangleNode : public RenderNode {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Color,Set=SetColor)
    private: Color color_;

    private: RenderMesh mesh_;

    public: RectangleNode(void);

    /// Set the color to render the sprite with
    public: void SetColor(Color color);

    protected: virtual void UpdateMesh (const Rect& r) override;

    protected: virtual bool DrawMesh (RenderContext* rc) override;
  };

} // namespace noz


#endif // __noz_RectangleNode_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_BorderNode_h__
#define __noz_BorderNode_h__

#include "RenderNode.h"

namespace noz {

  class BorderNode : public RenderNode {
    NOZ_OBJECT(EditorIcon="{F655932A-975C-4393-9681-15EC2C3B32B7}")

    NOZ_PROPERTY(Name=Color,Set=SetColor)
    private: Color color_;

    NOZ_PROPERTY(Name=Top,Set=SetTop)
    private: noz_float top_;

    NOZ_PROPERTY(Name=Left,Set=SetLeft)
    private: noz_float left_;

    NOZ_PROPERTY(Name=Right,Set=SetRight)
    private: noz_float right_;

    NOZ_PROPERTY(Name=Bottom,Set=SetBottom)
    private: noz_float bottom_;

    private: RenderMesh mesh_;

    public: BorderNode(void);

    public: void SetSize (noz_float size);

    public: void SetColor (Color color);

    public: void SetTop (noz_float v);
    public: void SetBottom (noz_float v);
    public: void SetRight (noz_float v);
    public: void SetLeft (noz_float v);

    protected: virtual void UpdateMesh (const Rect& r) override;

    protected: virtual bool DrawMesh (RenderContext* rc) override;
  };

} // namespace noz


#endif //__noz_BorderNode_h__



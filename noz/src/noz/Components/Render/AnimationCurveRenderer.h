///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Components_Render_AnimationCurveRenderer_h__
#define __noz_Components_Render_AnimationCurveRenderer_h__

#include "Renderer.h"
#include <noz/Animation/AnimationCurve.h>

namespace noz {

  class AnimationCurveRenderer : public Renderer {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=Curve)
    private: ObjectPtr<AnimationCurve> curve_;

    public: void SetCurve(AnimationCurve* curve) {curve_ = curve;}

    NOZ_FIXME()
    //public: virtual void Render(RenderContext* rc) override;
  };

} // namespace noz


#endif //__noz_Components_Render_AnimationCurveRenderer_h__



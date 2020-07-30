///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AnimationCurveRenderer.h"

using namespace noz;


/*
void AnimationCurveRenderer::Render(RenderContext* rc) {
#if 0
  if(curve_) {
    const Rect& r = GetNode()->GetRectangle();
    for(noz_int32 i= 0;i<25;i++) {
      noz_float t1 = i / 25.0f;
      noz_float t2 = (i+1) / 25.0f;
      gc->DrawLine(
        Vector2(
          r.x + r.w * t1,
          r.y + r.h * curve_->Evaluate(t1)
        ),
        Vector2(
          r.x + r.w * t2,
          r.y + r.h * curve_->Evaluate(t2)
        ),
        Color(0,255,0,255)
      );
    }
  }
#endif
}

*/

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_WeightedKeyFrame_h__
#define __noz_WeightedKeyFrame_h__

namespace noz {

  class WeightedKeyFrame : public KeyFrame {
    NOZ_OBJECT(Abstract)

    NOZ_PROPERTY(Name=InTangent)
    protected: noz_float in_tangent_;

    NOZ_PROPERTY(Name=OutTangent)
    protected: noz_float out_tangent_;

    public: WeightedKeyFrame(void);

    public: noz_float GetTangentOut(void) const {return out_tangent_;}

    public: noz_float GetTangentIn(void) const {return in_tangent_;}

    public: void SetTangentIn (noz_float t) {in_tangent_ = t;}

    public: void SetTangentOut (noz_float t) {out_tangent_ = t;}
  };

}

#endif // __noz_WeightedKeyFrame_h__

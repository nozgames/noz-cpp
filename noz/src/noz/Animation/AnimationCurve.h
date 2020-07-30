///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Animation_AnimationCurve_h__
#define __noz_Animation_AnimationCurve_h__

namespace noz {

  class AnimationCurve : public Asset {
    NOZ_OBJECT(Managed)

    public: struct Keyframe : public Object {
      NOZ_OBJECT()

      public: Keyframe(void) {
        time_ = 0; value_ = 0; in_tangent_ = 0; out_tangent_ = 0;         
      }
      public: Keyframe(noz_float t, noz_float v, noz_float intan, noz_float outtan) {
        time_ = t; value_ = v; in_tangent_ = intan; out_tangent_ = outtan;
      }

      NOZ_PROPERTY(Name=Time)
      public: noz_float time_;

      NOZ_PROPERTY(Name=Value)
      public: noz_float value_;

      NOZ_PROPERTY(Name=InTangent)
      public: noz_float in_tangent_;

      NOZ_PROPERTY(Name=OutTangent)
      public: noz_float out_tangent_;
    };

    NOZ_PROPERTY(Name=Keyframes)
    private: std::vector<Keyframe> keyframes_;

    public: AnimationCurve(void);

    /// Evalulate the animation curve for the time t
    public: noz_float Evaluate (noz_float t);

    /// Add a new keyframe
    public: void AddKey(const Keyframe& kf);

    /// Get the keyframe index for the given time.
    private: noz_uint32 GetKeyframeIndex(noz_float t);

    /// Evalulate the animation curve for the two given key frames
    private: noz_float Evaluate (noz_float t, const Keyframe& kf1, const Keyframe& kf2);
  };

} // namespace noz

#endif // __noz_Animation_AnimationCurve_h__

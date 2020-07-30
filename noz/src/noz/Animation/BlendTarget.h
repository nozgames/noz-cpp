///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_BlendTarget_h__
#define __noz_BlendTarget_h__

namespace noz {

  static const noz_uint32 MaxAnimationTracks = 4;

  class AnimationTrack;
  class AnimationLayer;

  class BlendTarget : public Object {
    NOZ_OBJECT(NoAllocator)

    protected: struct Blend {
      noz_float elapsed_;
      noz_float duration_;
      noz_float blend_elapsed_;
      noz_float blend_duration_;
      AnimationTarget* target_;
      AnimationLayer* layer_;
      Blend* prev_;
      Blend* next_;
      WrapMode wrap_;

      /// Last known frame for each track.  The structure will be allocated larger
      /// to accomidate the extra tracks if necessary.  
      /// IMPORTANT: This member must be last in the sturcture to ensure the extra
      /// allocated bytes will be in the proper location.
      noz_int32 frame_[1];
    };

    protected: Blend* blend_last_;
    protected: Blend* blend_first_;
    protected: Object* target_;
    protected: Property* target_property_;
    protected: bool clamped_;

    public: BlendTarget (Object* target, Property* target_property);

    public: ~BlendTarget (void);

    public: void* GetTarget(void) const {return target_;}

    public: Property* GetTargetProperty(void) const {return target_property_;}

    /// Stop all animation on the given layer
    public: void Stop (AnimationLayer* layer, noz_float blend_time);

    /// Play the given track on the given layer
    public: void Play (AnimationLayer* layer, AnimationTarget* target, noz_float duration=-1, WrapMode wrap=WrapMode::Clamp, noz_float blend_duration=0.0f);

    /// Advance the blend target by the given amount
    public: virtual void Advance (noz_float elapsed) = 0;

    /// Stop a blend with a given blend time
    protected: virtual bool BlendedStop (Blend* blend, noz_float blend_time) {return false;}

    /// Returns true if the blend target is still animating
    public: bool IsAnimating (void) const {return blend_first_ != nullptr;}

    /// Returns true if the blend target is still animating but the value is clamped. 
    public: bool IsClamped (void) const {return clamped_;}

    /// Stop the given blend immeidately
    protected: void Stop (Blend* blend);

    /// Update the frame index within the blend based on the elapsed time
    protected: void UpdateFrame (Blend* b);
  };

}

#endif // __noz_BlendTarget_h__

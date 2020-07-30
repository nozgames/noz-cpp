///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Animation_Animation_h__
#define __noz_Animation_Animation_h__

#include "AnimationTrack.h"
#include "AnimationTarget.h"

namespace noz {

  NOZ_ENUM() enum class WrapMode {
    Once,
    Loop,
    Clamp,
  };

  class EventKeyFrame;

  class Animation : public Asset {
    NOZ_OBJECT(Managed,EditorIcon="{16144FE1-4ACB-491A-B7E6-BD0A1305A6C9}")

    friend class AnimationTrack;
    friend class AnimationTarget;

    /// True if the animation should loop
    // TODO: WrapMode
    protected: NOZ_PROPERTY(Name=Looping) bool looping_;

    /// All unique targets within the animation
    protected: NOZ_PROPERTY(Name=Targets,Add=AddTarget) std::vector<AnimationTarget*> targets_;

    protected: NOZ_PROPERTY(Name=EventTrack,Set=SetEventTrack) AnimationTrack* event_track_;

    protected: NOZ_PROPERTY(Name=Wrap) WrapMode wrap_;

    /// Calcuated duration of the animation in seconds.
    protected: noz_float duration_;

    /// Default constructor
    public: Animation(void);

    /// Add an event key frame to the animation
    public: void AddEvent (EventKeyFrame* kf);

    /// Add the given animation target to the animation
    public: void AddTarget (AnimationTarget* target);

    /// Add a new target for the given property
    public: AnimationTarget* AddTarget (Property* p, const Guid& guid = Guid::Empty);

    /// Return the number of targets in the animation
    public: noz_uint32 GetTargetCount (void) const {return targets_.size();}

    /// Return the target at the given index
    public: AnimationTarget* GetTarget (noz_uint32 i) const {return targets_[i];}

    /// Return the target matching the given globally unique identifier
    public: AnimationTarget* GetTarget (const Guid& guid) const;

    /// Return the total duration of the animation in seconds.
    public: noz_float GetDuration(void) const {return duration_;}

    /// Return the wrap mode of the animation
    public: WrapMode GetWrap (void) const {return wrap_;}

    public: AnimationTrack* GetEventTrack (void) const {return event_track_;}

    /// Remove the target from the animation and delete its associated memory
    public: void RemoveTarget (AnimationTarget* target);

    /// Release the target from the animation by removing it from the animations list but 
    /// instead of freeing the target the target is released to the caller.
    public: void ReleaseTarget (AnimationTarget* target);

    /// Return true if the animation is looping.
    public: bool IsLooping(void) const {return looping_;}

    private: void UpdateDuration (void);

    private: void SetEventTrack (AnimationTrack* track);
  };
}

#endif // __noz_Animation_Animation_h__

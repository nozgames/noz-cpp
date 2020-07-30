///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AnimationTarget_h__
#define __noz_AnimationTarget_h__

namespace noz {

  class Animation;
  class BlendTarget;
  class AnimationTrack;

  class AnimationTarget : public Object {
    NOZ_OBJECT()

    friend class Animation;
    friend class AnimationTrack;

    /// Globally unique identifier of the target
    private: NOZ_PROPERTY(Name=Guid) Guid guid_;

    /// Property the target modifies
    private: NOZ_PROPERTY(Name=TargetProperty) Property* target_property_;

    /// Tracks that perform the animation on the target.  For most propertieis there will
    /// be a single track but for properties with multiple animation channels there will 
    /// be a track for each channel.
    private: NOZ_PROPERTY(Name=Tracks,Add=AddTrack) std::vector<AnimationTrack*> tracks_;

    /// Optional name of the target..
    private: NOZ_PROPERTY(Name=Name) Name name_;

    /// Backwards reference to animation that owns the target.
    private: Animation* animation_;

    /// Cumulative duration of all tracks on this target
    private: noz_float duration_;

    /// Default constructor
    public: AnimationTarget (void);

    /// Construct a target from a property
    public: AnimationTarget (Property* p, const Guid& guid);

    /// Return the globally unique identifier of the target
    public: const Guid& GetGuid (void) const {return guid_;}

    /// Return the target property 
    public: Property* GetTargetProperty (void) const {return target_property_;}

    /// Add a new track to the animation target
    public: void AddTrack (AnimationTrack* track);

    /// Remove the given track from the target and free all associated memory
    public: void RemoveTrack (AnimationTrack* track);

    /// remove the given track from the target but release control of the track object to the caller
    public: void ReleaseTrack (AnimationTrack* track);

    /// Return the duration in seconds of the the target.
    public: noz_float GetDuration (void) const {return duration_;}

    /// Return the number of tracks in the animation target
    public: noz_uint32 GetTrackCount (void) const {return tracks_.size();}

    /// Return the track by index.
    public: AnimationTrack* GetTrack (noz_uint32 index) const {return tracks_[index];}

    /// Return the track that matches the given identifier
    public: AnimationTrack* GetTrackById (noz_uint32 id) const;

    /// Create a blend target from the animation target
    public: BlendTarget* CreateBlendTarget(Object* target);

    public: void SetName (const Name& name) {name_ = name;}

    public: const Name& GetName (void) const {return name_;}

    public: Animation* GetAnimation(void) const {return animation_;}

    /// Update the duration of the target using the current track list
    protected: void UpdateDuration (void);

    protected: virtual void OnDeserialized (void) override;
  };
}

#endif // __noz_AnimationTarget_h__

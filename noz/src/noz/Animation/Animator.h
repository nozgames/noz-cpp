///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Animation_Animator_h__
#define __noz_Animation_Animator_h__

#include "AnimatorLayer.h"
#include "AnimationController.h"

namespace noz {

  class BlendTarget;
  class Animator;

  class AnimatorTarget : public Object {
    NOZ_OBJECT()

    friend class Animator;

    NOZ_PROPERTY(Name=Guid)
    private: Guid guid_;

    private: NOZ_PROPERTY(Name=Object) ObjectPtr<Object> object_;

    public: AnimatorTarget (void) { }

    public: AnimatorTarget (const Guid& g, Object* t) : guid_(g), object_(t) { }

    public: const Guid& GetGuid (void) const {return guid_;}

    public: Object* GetTarget (void) const {return object_;}

    public: void SetTarget (Object* t) {object_ = t;}
  };

  class Animator : public Component {
    NOZ_OBJECT(EditorIcon="{16144FE1-4ACB-491A-B7E6-BD0A1305A6C9}")

    NOZ_TODO("Global list of BlendTargets that the frame can just quickly process.  Local vector will store");

    /// Animator layers.
    protected: NOZ_PROPERTY(Name=Layers,Private,Add=AddLayer) std::vector<AnimatorLayer*> layers_;
    protected: NOZ_PROPERTY(Name=Controller,Set=SetController) ObjectPtr<AnimationController> controller_;
    protected: NOZ_PROPERTY(Name=Targets,Private) std::vector<AnimatorTarget*> targets_;

    /// Currently executing blend targets
    protected: std::vector<BlendTarget*> blends_;

    /// Vector of current states per layer
    protected: std::vector<AnimationState*> current_states_;

    /// Timescale for animation
    protected: noz_float timescale_;

    protected: Name pending_state_;

    protected: bool advancing_;

    /// True if all the targets within the animator are clamped
    protected: bool clamped_;

    /// Default constructor
    public: Animator(void);

    /// Default destructor
    public: ~Animator(void);
    
    /// Returns true if the animator is currently animating.  This value will
    /// be true if an animation is running but the values are clamped as well.  To determine
    /// if all values are clamped call IsClamped
    public: bool IsAnimating (void) const {return !blends_.empty();}

    /// Returns true if the animator is currently animating but all of the targets are clamped.
    public: bool IsClamped (void) const {return IsAnimating() && clamped_;}

    /// Add a new layer to the animator
    public: void AddLayer (AnimatorLayer* layer);

    /// Set the animation state 
    public: void SetState (const Name& state);

    public: void SetController (AnimationController* controller);

    public: AnimationController* GetController (void) const {return controller_;}

    /// continue to animate the animator
    public: virtual void Animate(void);

    public: void Stop (void);

    public: noz_uint32 GetTargetCount (void) const {return targets_.size();}

    public: AnimatorTarget* GetTarget (noz_uint32 i) const {return targets_[i];}

    public: AnimatorTarget* GetTarget (const Guid& guid) const;

    public: AnimatorTarget* GetTarget (Animation* animation, Object* o) const;

    public: void AddTarget (AnimatorTarget* animator_target);

    public: AnimatorTarget* AddTarget (const Guid& guid, Object* target);

    public: void RemoveTarget (noz_uint32 index);

    public: void RemoveTarget (AnimatorTarget* target);

    public: void ReleaseTarget (AnimatorTarget* target);

    /// Advance time within the adnimator by the given elapsed time
    public: void Advance (noz_float elapsed);

    private: virtual void OnAwake (void) override;
  };

} // namespace noz


#endif //__noz_Animation_Animator_h__



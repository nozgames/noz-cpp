///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AnimationLayer_h__
#define __noz_AnimationLayer_h__

#include "AnimationState.h"

namespace noz {

  class AnimationLayer : public Object {
    NOZ_OBJECT()
    
    friend class AnimationController;

    NOZ_PROPERTY(Name=Name)
    private: Name name_;

    NOZ_PROPERTY(Name=States,Add=AddState)
    private: std::vector<ObjectPtr<AnimationState>> states_;

    NOZ_PROPERTY(Name=Weight)
    private: noz_float weight_;

    /// Index of the layer within the controller
    private: noz_uint32 index_;

    /// Default constructor
    public: AnimationLayer(void);

    /// Default destructor
    public: ~AnimationLayer(void);

    public: void AddState (AnimationState* state);

    public: noz_float GetWeight (void) const {return weight_;}

    /// Return the number of states in the layer
    public: noz_uint32 GetStateCount (void) const {return states_.size();}

    /// Return the state at the given index.
    public: AnimationState* GetState (noz_uint32 i) const {return states_[i];}

    /// Return the state that matches the given name
    public: AnimationState* GetState (const Name& name) const;

    /// Return the animation layer name
    public: const Name& GetName (void) const {return name_;}

    /// Return the index of the layer within the controller
    public: noz_uint32 GetIndex (void) const {return index_;}
  };
}

#endif // __noz_AnimationLayer_h__

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_AnimationController_h__
#define __noz_AnimationController_h__

#include "AnimationLayer.h"

namespace noz {

  class AnimationController : public Asset {
    NOZ_OBJECT(Managed,EditorIcon="{F87F00E5-A8E4-46C0-992E-5C2A9B18B130}")

    NOZ_PROPERTY(Name=Layers,Add=AddLayer)
    private: std::vector<ObjectPtr<AnimationLayer>> layers_;
    
    /// Default constructor
    public: AnimationController(void);

    /// Destructor
    public: ~AnimationController (void);

    /// Return the number of layers in the controller
    public: noz_uint32 GetLayerCount (void) const {return layers_.size();}

    /// Return the layer at the given index.
    public: AnimationLayer* GetLayer (noz_uint32 i) const {return layers_[i];}    

    public: void AddLayer (AnimationLayer* layer);
  };
}

#endif // __noz_AnimationController_h__

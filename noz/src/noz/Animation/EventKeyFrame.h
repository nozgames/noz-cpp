///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_EventKeyFrame_h__
#define __noz_EventKeyFrame_h__

#include "KeyFrame.h"

namespace noz {

  class EventKeyFrame : public KeyFrame {
    NOZ_OBJECT(EditorIcon="{B06F9534-55F5-4DD4-8C75-D90F863593EC}")

    private: NOZ_PROPERTY(Name=Method) Method* method_;
    private: NOZ_PROPERTY(Name=AnimationState) Name animation_state_;
    private: NOZ_PROPERTY(Name=AudioClip) ObjectPtr<AudioClip> audio_clip_;

    public: EventKeyFrame(void);

    public: Method* GetMethod (void) const {return method_;}

    public: void SetMethod (Method* method) {method_ = method;}

    public: AudioClip* GetAudioClip(void) const {return audio_clip_;}

    public: void SetAudioClip (AudioClip* clip) {audio_clip_ = clip;}    

    public: const Name& GetAnimationState (void) const {return animation_state_;}

    public: void SetAnimationState (const Name& state) {animation_state_ = state;}

    public: void Fire (Object* target);

    /// Return the property type the EventKeyFrame requires
    public: virtual Type* GetPropertyType(void) const override {return typeof(ObjectPtrProperty);}

    public: virtual Type* GetValueType(void) const override { return typeof(Sprite); }

    /// Sprites do not support weights.
    public: virtual bool IsWeighted (void) const override {return false;}
  };

}

#endif // __noz_EventKeyFrame_h__

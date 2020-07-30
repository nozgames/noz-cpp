///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Behavior_h__
#define __noz_Behavior_h__

namespace noz {

  class Behavior : public Component, public EventObserver {
    NOZ_OBJECT(Abstract)

    private: bool started_;

    public: Behavior(void);

    private: virtual void Update(void) final;
    private: virtual void LateUpdate(void) final;
    private: virtual void FixedUpdate(void) final;

    protected: virtual void OnStart(void) { }
    protected: virtual void OnUpdate(void) { }
    protected: virtual void OnLateUpdate(void) {}
    protected: virtual void OnFixedUpdate(void) {}
  };

} // namespace noz


#endif //__noz_Behavior_h__



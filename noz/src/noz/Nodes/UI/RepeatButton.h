///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RepeatButton_h__
#define __noz_RepeatButton_h__

#include "ButtonBase.h"

namespace noz {

  class RepeatButton : public ButtonBase {
    NOZ_OBJECT(DefaultStyle="{7817AB26-EC04-4F23-A94E-B44AF7730B77}")

    NOZ_PROPERTY(Name=Delay)
    private: noz_float delay_;

    NOZ_PROPERTY(Name=Interval)
    private: noz_float interval_;

    private: noz_float next_click_;

    /// Default constructor
    public: RepeatButton (void);

    public: virtual void OnClick (void) override;
    public: virtual void OnMouseDown (SystemEvent* e) override;
    public: virtual void OnMouseUp (SystemEvent* e) override;
    public: virtual void Update (void) override;

  };

} // namespace noz


#endif //__noz_RepeatButton_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ToggleButton_h__
#define __noz_ToggleButton_h__

#include "ButtonBase.h"

namespace noz {

  typedef Event<> CheckedEventHandler;
  typedef Event<> UncheckedEventHandler;

  class ToggleButton : public ButtonBase {
    NOZ_OBJECT(DefaultStyle="{8AF62F49-F8D8-48C3-9885-0E6EEF202C2A}")

    /// Event which is triggered when the toggle button is checked or unchecked
    public: CheckedEventHandler Checked;
    public: UncheckedEventHandler Unchecked;;

    /// Default ToggleButton constructor
    public: ToggleButton (void);

    /// True if the ToggleButton is in the checked state
    private: bool checked_;

    /// Set the checked state of the ToggleButton
    public: void SetChecked (bool checked);

    public: bool IsChecked (void) const {return checked_;}

    public: virtual void OnClick (void) override;

    private: virtual void UpdateAnimationState (void) override;
  };

} // namespace noz


#endif //__noz_ToggleButton_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RadioButton_h__
#define __noz_RadioButton_h__

#include "ToggleButton.h"

namespace noz {

  class RadioButton : public ToggleButton {
    NOZ_OBJECT(DefaultStyle="{EBCF3BE0-E9F2-4FFB-BA2B-A898954D979F}")

    /// Default RadioButton constructor
    public: RadioButton (void);

    public: virtual void OnClick (void) override;
  };

} // namespace noz


#endif //__noz_RadioButton_h__


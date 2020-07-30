///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_CheckBox_h__
#define __noz_CheckBox_h__

#include "ToggleButton.h"

namespace noz {

  class CheckBox : public ToggleButton {
    NOZ_OBJECT(DefaultStyle="{833F1A93-7BCE-4B18-AD97-285A04C0210B}")

    /// Default CheckBox constructor
    public: CheckBox (void);
  };

} // namespace noz


#endif //__noz_CheckBox_h__


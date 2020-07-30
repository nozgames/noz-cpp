///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Components_Button_h__
#define __noz_Components_Button_h__

#include "ButtonBase.h"

namespace noz {  

  class Button : public ButtonBase {
    NOZ_OBJECT(DefaultStyle="{7817AB26-EC04-4F23-A94E-B44AF7730B77}")

    /// Default button constructor
    public: Button (void);
  };

} // namespace noz


#endif //__noz_Components_Button_h__


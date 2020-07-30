///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SearchTextBox_h__
#define __noz_SearchTextBox_h__

#include "TextBox.h"

namespace noz {

  class Button;

  class SearchTextBox : public TextBox {
    NOZ_OBJECT(DefaultStyle="{DD933D3A-E644-4F3B-B2E9-F2860E84FFD4}")

    NOZ_CONTROL_PART(Name=Button)
    private: ObjectPtr<Button> button_;

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnButtonClick (UINode* sender);
  };

} // namespace noz


#endif //__noz_SearchTextBox_h__


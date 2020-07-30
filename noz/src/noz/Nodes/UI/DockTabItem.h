///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DockTabItem_h__
#define __noz_DockTabItem_h__

#include "TabItem.h"

namespace noz {

  class TextNode;
  class DockItem;

  class DockTabItem : public TabItem {
    NOZ_OBJECT(DefaultStyle="{BF6950C9-2099-4A1C-8574-BCCAFDFD3772}")
    
    friend class DockManager;
    friend class DockItem;
    friend class DockPane;

    private: ObjectPtr<DockItem> item_;

    /// Default constructor
    public: DockTabItem(void);

    /// Default destructor
    public: ~DockTabItem (void);
  };

} // namespace noz


#endif //__noz_DockTabItem_h__


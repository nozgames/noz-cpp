///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_ConsoleView_h__
#define __noz_Editor_ConsoleView_h__

#include <noz/Nodes/UI/ListView.h>

namespace noz {
namespace Editor {

  class ConsoleView : public Control {
    NOZ_OBJECT(DefaultStyle="{743665BC-59E1-4C5F-A4DD-1F37ECFC0A0C}")

    NOZ_CONTROL_PART(Name=ListView)
    private: ObjectPtr<ListView> list_view_;

    public: ConsoleView (void);

    public: ~ConsoleView (void);

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnConsoleMessage (ConsoleMessageType type, Object* context, const char* msg);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_ConsoleView_h__


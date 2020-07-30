///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_DeployDialog_h__
#define __noz_Editor_DeployDialog_h__

#include <noz/System/Process.h>
#include <noz/Nodes/UI/DialogBox.h>
#include <noz/Nodes/UI/ListView.h>
#include <noz/Nodes/UI/ProgressBar.h>
#include <noz/Editor/Tool/Makefile.h>

namespace noz {
namespace Editor {

  class DeployProgressDialog : public DialogBox {
    NOZ_OBJECT(DefaultStyle="{32AA0488-EB84-4748-B043-F6CB050E3923}")

    NOZ_CONTROL_PART(Name=CancelButton)
    private: ObjectPtr<Button> cancel_button_;

    NOZ_CONTROL_PART(Name=ProgressBar)
    private: ObjectPtr<ProgressBar> progress_bar_;

    private: Process* process_;

    public: DeployProgressDialog(Process* process);

    public: ~DeployProgressDialog(void);

    public: virtual void Update (void) override;
  };

  class DeployDialog : public DialogBox {
    NOZ_OBJECT(DefaultStyle="{B0A661EE-23C1-44FB-AD4A-DCF86E1D42E2}")

    NOZ_CONTROL_PART(Name=Platforms)
    private: ObjectPtr<ListView> platforms_;

    NOZ_CONTROL_PART(Name=OKButton)
    private: ObjectPtr<Button> ok_button_;

    NOZ_CONTROL_PART(Name=CancelButton)
    private: ObjectPtr<Button> cancel_button_;

    private: Makefile::PlatformType platform_;

    public: DeployDialog (void);

    public: ~DeployDialog (void);

    public: Makefile::PlatformType GetPlatform (void) const {return platform_;}

    protected: virtual bool OnApplyStyle (void) override;

    private: void OnOk (UINode*);
    private: void OnCancel (UINode*);
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_DeployDialog_h__


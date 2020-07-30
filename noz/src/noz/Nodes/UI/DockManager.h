///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DockManager_h__
#define __noz_DockManager_h__

#include "Splitter.h"
#include "Thumb.h"
#include "DockPane.h"

namespace noz {

  class DockTabPane;

  class DockThumb : public Thumb {
    NOZ_OBJECT(DefaultStyle="{5CB0A1A3-B32E-4E2E-A44D-437B21F65144}")
  };

  class DockManager : public Control {
    NOZ_OBJECT(DefaultStyle="{49383049-835F-457E-A638-A31AB445BE61}")

    friend class DockPane;      

    NOZ_CONTROL_PART(Name=PanesContainer)
    private: ObjectPtr<Node> panes_container_;

    private: ObjectPtr<Node> content_container_;

    private: ObjectPtr<Node> undocked_root_;

    private: ObjectPtr<DockPane> selected_;

    private: bool dock_invalid_;

    public: DockManager(void);

    public: ~DockManager (void);

    public: DockPane* GetSelected (void) const {return selected_;}

    private: void DockPanes (void);
    private: void UnDockPanes (void);

    private: Splitter* CreateSplitter (DockStyle style);

    private: void InvalidateDock (void);

    private: void Select (DockPane *item);

    protected: virtual Vector2 Measure (const Vector2& a) override;
    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnChildAdded(Node * child) override;
    protected: virtual void OnChildRemoved(Node * child) override;
    protected: virtual void OnStyleChanged (void) override;

  };

} // namespace noz


#endif //__noz_DockManager_h__


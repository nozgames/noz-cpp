///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AssetBrowserListViewItem_h__
#define __noz_Editor_AssetBrowserListViewItem_h__

#include <noz/Nodes/UI/ListViewItem.h>

namespace noz {
namespace Editor {

  class AssetBrowser;

  class AssetBrowserListViewItem : public ListViewItem {
    NOZ_OBJECT(DefaultStyle="{900EFD6E-22A6-48A7-8C1D-DB0A4C28BB55}")
    
    friend class AssetBrowser;

    private: AssetFile* file_;
    private: AssetFolder* folder_;
    private: AssetBrowser* browser_;

    public: AssetBrowserListViewItem (AssetBrowser* browser, AssetFolder* folder);
    public: AssetBrowserListViewItem (AssetBrowser* browser, AssetFile* file);

    public: ~AssetBrowserListViewItem (void);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnMouseDown (SystemEvent* e) override;
    protected: virtual void OnMouseOver (SystemEvent* e) override;
    protected: virtual void OnMouseUp (SystemEvent* e) override;

    protected: virtual void OnOpen (void) override;
  };


} // namespace Editor
} // namespace noz


#endif // __noz_Editor_AssetBrowserListViewItem_h__


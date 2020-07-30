///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AssetBrowserTreeViewItem_h__
#define __noz_Editor_AssetBrowserTreeViewItem_h__

#include <noz/Nodes/UI/TreeViewItem.h>

namespace noz {
namespace Editor {  

  class AssetFolder;
  class AssetBrowser;

  class AssetBrowserTreeViewItem : public TreeViewItem {
    NOZ_OBJECT(DefaultStyle="{1C6949C5-E52C-486E-B5D5-814BABB9A4FE}")

    friend class AssetBrowser;

    NOZ_CONTROL_PART(Name=NameNode)
    private: ObjectPtr<TextNode> name_node_;

    NOZ_CONTROL_PART(Name=IconNode)
    private: ObjectPtr<SpriteNode> icon_node_;

    private: AssetBrowser* browser_;

    private: AssetFolder* folder_;

    public: AssetBrowserTreeViewItem (void);

    public: ~AssetBrowserTreeViewItem (void);

    public: void SetFolder (AssetFolder* folder);

    protected: virtual bool OnApplyStyle (void) override;

    protected: virtual void OnDragDrop (DragDropEventArgs* e) override;
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_AssetBrowserTreeViewItem_h__


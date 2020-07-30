///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AssetBrowser_h__
#define __noz_Editor_AssetBrowser_h__

#include "AssetBrowserListViewItem.h"
#include "AssetBrowserTreeViewItem.h"

namespace noz {

  class TreeView;
  class ListView;

namespace Editor {

  class AssetBrowser : public Control {
    NOZ_OBJECT(DefaultStyle="{AFF53733-ED3D-480A-8B8D-414F79A994FA}")

    friend class AssetBrowserListViewItem;

    private: NOZ_CONTROL_PART(Name=TreeView) ObjectPtr<TreeView> tree_view_;
    private: NOZ_CONTROL_PART(Name=ListViewDetails) ObjectPtr<ListView> list_view_details_;
    private: NOZ_CONTROL_PART(Name=ListViewThumbnail) ObjectPtr<ListView> list_view_thumbnail_;
    private: NOZ_CONTROL_PART(Name=NewAssetPopup) ObjectPtr<Popup> new_asset_popup_;
    private: NOZ_CONTROL_PART(Name=NewAssetButton) ObjectPtr<Button> new_asset_button_;
    private: NOZ_CONTROL_PART(Name=NewFolderButton) ObjectPtr<Button> new_folder_button_;
    private: NOZ_CONTROL_PART(Name=NewAssetTypePicker) ObjectPtr<TypePicker> new_asset_type_picker_;
    
    private: ObjectPtr<ListView> list_view_;

    private: noz_float list_view_zoom_;

    public: AssetBrowser (void);

    public: ~AssetBrowser (void);

    public: void Refresh (void);

    public: void SelectFolder (AssetFolder* folder);

    private: AssetBrowserTreeViewItem* FindTreeViewItem (AssetFolder* folder, AssetBrowserTreeViewItem* tvitem);
    private: AssetBrowserListViewItem* FindListViewItem (AssetFolder* folder);
    private: AssetBrowserListViewItem* FindListViewItem (AssetFile* file);

    private: AssetBrowserTreeViewItem* CreateTreeViewItem (AssetFolder* folder);

    private: void UpdateListView (void);

    private: static noz_int32 SortFiles (const Node* lvitem1, const Node* lvitem2);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnKeyDown (SystemEvent* e) override;

    private: void DeleteSelectedItems (void);
    private: void DuplicateSelectedItems (void);

    private: bool GetSelectedItems (std::vector<AssetFile*>& files, std::vector<AssetFolder*>& folders);

#if 0
    private: void HandleListViewDragDrop (Object* o);
    private: AssetFile* CreateFileFromObject (Object* o, AssetFolder* folder);
#endif

    private: void OnTreeViewSelectionChanged (UINode* sender);
    private: void OnListViewTextChanged(UINode* sender, ListViewItem* item, const String* text);
    private: void OnListViewDragDrop(UINode* sender, DragDropEventArgs* e);
    private: void OnNewAssetButtonClicked (UINode* sender);
    private: void OnNewFolderButtonClicked (UINode* sender);
    private: void OnNewAssetTypeSelected(UINode* sender, Type* type);

  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_AssetBrowser_h__


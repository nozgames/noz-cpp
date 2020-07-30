///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AssetBrowser.h"

#include <noz/Nodes/UI/Button.h>
#include <noz/Nodes/UI/Splitter.h>
#include <noz/Nodes/UI/ListView.h>
#include <noz/Nodes/UI/TreeView.h>
#include <noz/Nodes/UI/TreeViewItem.h>
#include <noz/Nodes/UI/MessageBox.h>

#include <noz/Editor/Nodes/UI/AssetBrowserTreeViewItem.h>
#include <noz/Editor/Nodes/UI/AssetBrowserListViewItem.h>

using namespace noz;
using namespace noz::Editor;

AssetBrowser::AssetBrowser (void) {
  list_view_zoom_ = 0.0f;
}

AssetBrowser::~AssetBrowser(void) {
}

bool AssetBrowser::OnApplyStyle(void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == tree_view_) return false;

  if(new_asset_button_) new_asset_button_->Click += ClickEventHandler::Delegate(this,&AssetBrowser::OnNewAssetButtonClicked);
  if(new_folder_button_) new_folder_button_->Click += ClickEventHandler::Delegate(this,&AssetBrowser::OnNewFolderButtonClicked);
  if(new_asset_type_picker_) new_asset_type_picker_->SelectionChanged += TypeSelectedEventHandler::Delegate(this,&AssetBrowser::OnNewAssetTypeSelected);

  tree_view_->SelectionChanged += SelectionChangedEventHandler::Delegate(this,&AssetBrowser::OnTreeViewSelectionChanged);

  if(list_view_thumbnail_) {
    list_view_thumbnail_->ItemTextChanged  += ListViewTextChangedEventHandler::Delegate(this, &AssetBrowser::OnListViewTextChanged);
    list_view_thumbnail_->SetDragDropTarget(true);
    list_view_thumbnail_->DragDrop += DragDropEventHandler::Delegate(this, &AssetBrowser::OnListViewDragDrop);
    list_view_ = list_view_thumbnail_;
  }  

  if(list_view_details_) {
    list_view_details_->ItemTextChanged  += ListViewTextChangedEventHandler::Delegate(this, &AssetBrowser::OnListViewTextChanged);
    list_view_details_->SetDragDropTarget(true);
    list_view_details_->DragDrop += DragDropEventHandler::Delegate(this, &AssetBrowser::OnListViewDragDrop);
    if(list_view_thumbnail_) list_view_thumbnail_->SetVisibility(Visibility::Collapsed);
    list_view_ = list_view_details_;
    list_view_->SetVisibility(Visibility::Visible);
  }

  Refresh();

  return true;
}

void AssetBrowser::Refresh (void) {
  // Need both tree view and list view to populate..
  if(tree_view_ == nullptr || list_view_ == nullptr) return;

    // Reset the tree
  tree_view_->RemoveAllChildren();

  AssetBrowserTreeViewItem* tvitem = CreateTreeViewItem(AssetDatabase::GetRootFolder());
  if(tvitem) {
    tvitem->browser_ = this;
    tree_view_->AddChild(tvitem);
    tvitem->SetExpanded(true);
    tvitem->SetSelected(true);
  }
}

AssetBrowserTreeViewItem* AssetBrowser::CreateTreeViewItem (AssetFolder* folder) {
  noz_assert(folder);

  AssetBrowserTreeViewItem* tvitem = new AssetBrowserTreeViewItem;
  tvitem->browser_ = this;

  // Set the folder on the item
  tvitem->SetFolder(folder);

  // Add sub folders as items.
  for(auto it=folder->GetFolders().begin(); it!=folder->GetFolders().end(); it++) {
    AssetBrowserTreeViewItem* tvitem_child = CreateTreeViewItem(*it);
    if(tvitem_child) tvitem->AddChild(tvitem_child);
  }  

  return tvitem;
}


void AssetBrowser::OnTreeViewSelectionChanged (UINode* sender) {
  noz_assert(tree_view_);

  UpdateListView();
}

void AssetBrowser::OnListViewDragDrop(UINode* sender, DragDropEventArgs* args) {
  if(args->GetEventType() == DragDropEventType::Enter) {
    if(args->GetObject<AssetFile>()) {
      args->SetEffects(DragDropEffects::Move);
    }
  }
}

void AssetBrowser::OnListViewTextChanged(UINode* sender, ListViewItem* item, const String* text) {
  AssetBrowserListViewItem* lvitem = (AssetBrowserListViewItem*)item;
  if(lvitem->folder_) {
    if(AssetDatabase::MoveFolder (lvitem->folder_, text->ToCString())) {
      UpdateListView();

      TreeViewItem* tvitem = FindTreeViewItem(lvitem->folder_, nullptr);
      if(tvitem) {
        tvitem->SetText(lvitem->folder_->GetName());
      }
    }
  } else {
    AssetFile* file = lvitem->file_;

    // Move the file to the new name within the same folder.
    if(AssetDatabase::MoveFile (file, file->GetFolder(), text->ToCString())) {
      // Update the list view to reflect the change.
      UpdateListView();

      AssetBrowserListViewItem* lvitem = FindListViewItem(file);
      if(lvitem) {
        lvitem->SetSelected(true);
        lvitem->BringIntoView();
        list_view_->SetFocus();
      }
    }
  }
}

noz_int32 AssetBrowser::SortFiles (const Node* lvitem1, const Node* lvitem2) {
  AssetBrowserListViewItem* ablvitem1 = (AssetBrowserListViewItem*)lvitem1;
  AssetBrowserListViewItem* ablvitem2 = (AssetBrowserListViewItem*)lvitem2;
  if(ablvitem1->folder_ && !ablvitem2->folder_) return -1;
  if(ablvitem2->folder_ && !ablvitem1->folder_) return 1;
  if(ablvitem2->folder_ && ablvitem1->folder_) {
    return ablvitem1->folder_->GetName().ToString().CompareTo(ablvitem2->folder_->GetName().ToString(),StringComparison::OrdinalIgnoreCase);
  }
  noz_assert(ablvitem1->file_ && ablvitem2->file_);

  return ablvitem1->file_->GetName().ToString().CompareTo(ablvitem2->file_->GetName().ToString(),StringComparison::OrdinalIgnoreCase);
}

void AssetBrowser::UpdateListView (void) {
  auto items = tree_view_->GetSelectedItems();

  // Reset list
  list_view_->RemoveAllChildren();

  // Rebuild list.
  for(auto it=items.begin(); it!=items.end(); it++) {
    AssetBrowserTreeViewItem* abtvitem = (AssetBrowserTreeViewItem*)(TreeViewItem*)(*it);
    noz_assert(abtvitem);

    auto folders=abtvitem->folder_->GetFolders();
    for(auto itf=folders.begin(); itf!=folders.end(); itf++) {
      AssetFolder* folder = (*itf);
      noz_assert(folder);
      
      list_view_->AddChild(new AssetBrowserListViewItem(this,folder));
    }

    for(auto itfile=abtvitem->folder_->GetFiles().begin(); itfile!=abtvitem->folder_->GetFiles().end(); itfile++) {
      AssetFile* file = (*itfile);
      noz_assert (file);

      list_view_->AddChild(new AssetBrowserListViewItem(this,file));
    }
  }

  // Sort the list view items
  list_view_->SortChildren(&AssetBrowser::SortFiles);
}

void AssetBrowser::SelectFolder (AssetFolder* folder) {
  AssetBrowserTreeViewItem* tvitem = FindTreeViewItem(folder,nullptr);
  if(tvitem) {
    tree_view_->UnselectAll();
    tree_view_->Select(tvitem);
    tvitem->BringIntoView();
  }
}

AssetBrowserListViewItem* AssetBrowser::FindListViewItem (AssetFolder* folder) {
  if(list_view_ == nullptr) return nullptr;

  for(AssetBrowserListViewItem* lvitem=(AssetBrowserListViewItem*)list_view_->GetFirstChildItem(); 
      lvitem;
      lvitem = (AssetBrowserListViewItem*)lvitem->GetNextSiblingItem()) {
    if(lvitem->folder_ == folder) return lvitem;
  }
  return nullptr;
}

AssetBrowserListViewItem* AssetBrowser::FindListViewItem (AssetFile* file) {
  if(list_view_ == nullptr) return nullptr;

  for(AssetBrowserListViewItem* lvitem=(AssetBrowserListViewItem*)list_view_->GetFirstChildItem(); 
      lvitem;
      lvitem = (AssetBrowserListViewItem*)lvitem->GetNextSiblingItem()) {
    if(lvitem->file_ == file) return lvitem;
  }
  return nullptr;
}

AssetBrowserTreeViewItem* AssetBrowser::FindTreeViewItem (AssetFolder* folder, AssetBrowserTreeViewItem* tvitem) {
  AssetBrowserTreeViewItem* result = nullptr;

  TreeViewItem* tvchild = nullptr;
  if(tvitem) {
    if(tvitem->folder_ == folder) return tvitem;
    tvchild = tvitem->GetFirstChildItem();
  } else {
    tvchild = tree_view_->GetFirstChildItem();
  }

  for(; !result && tvchild; tvchild=tvchild->GetNextSiblingItem()) {
    noz_assert(tvchild->IsTypeOf(typeof(AssetBrowserTreeViewItem)));
    result = FindTreeViewItem(folder,(AssetBrowserTreeViewItem*)tvchild);
  }

  return result;
}

void AssetBrowser::OnNewAssetButtonClicked (UINode* sender) {
  if(nullptr == new_asset_popup_) return;  
  
  new_asset_type_picker_->Clear();
  new_asset_popup_->Open();
  new_asset_type_picker_->SetFocus();
}

void AssetBrowser::OnNewFolderButtonClicked (UINode* sender) {
  // Close the new asset popup..
  new_asset_popup_->Close();

  // The focused item within the tree view is where the new folder will be added..
  AssetBrowserTreeViewItem* focused_item = (AssetBrowserTreeViewItem*)tree_view_->GetFocusedItem();
  
  // If there is no focused item then select the root folder..
  if(nullptr == focused_item) {
    noz_assert(tree_view_->GetSelectedItem()==nullptr);
    focused_item = (AssetBrowserTreeViewItem*)  tree_view_->GetFirstChildItem();
    if(nullptr == focused_item) return;
    focused_item->SetSelected(true);
  }

  // Create the new folder within the focused item folder
  AssetFolder* folder = AssetDatabase::CreateFolder (focused_item->folder_, "New Folder");
  if(nullptr == folder) return;

  // Now that the folder has been created update the list view to ensure a 
  // list view item is created for it.
  UpdateListView();

  // Create a new tree view item for the folder
  AssetBrowserTreeViewItem* tvitem = CreateTreeViewItem(folder);
  if(nullptr == tvitem) return;

  // Add the item to the tree and automatically expand the parent tree item
  focused_item->AddChild(tvitem);
  focused_item->SetExpanded(true);

  // Find the list view item that was created for the new folder, select it, and
  // start editing its text
  AssetBrowserListViewItem* lvitem = FindListViewItem(folder);
  if(lvitem) {
    list_view_->UnselectAll();
    lvitem->SetSelected(true);
    lvitem->SetFocus();
    lvitem->BeginTextEdit();
  }
}

void AssetBrowser::OnNewAssetTypeSelected(UINode* sender, Type* type) {
  noz_assert(type);

  // Close the new asset popup
  new_asset_popup_->Close();

  // The focused item within the tree view is where the new folder will be added..
  AssetBrowserTreeViewItem* focused_item = (AssetBrowserTreeViewItem*)tree_view_->GetFocusedItem();
  
  // If there is no focused item then select the root folder..
  if(nullptr == focused_item) {
    noz_assert(tree_view_->GetSelectedItem()==nullptr);
    focused_item = (AssetBrowserTreeViewItem*)  tree_view_->GetFirstChildItem();
    if(nullptr == focused_item) return;
    focused_item->SetSelected(true);
  }

  // Create the new asset
  AssetFile* file = AssetDatabase::CreateFile(focused_item->folder_,type,String::Format("New %s", type->GetName().ToCString()).ToCString());
  if(nullptr == file) { 
    Console::WriteError("failed to create %s asset", type->GetName().ToCString());
    return;
  }

  // Refresh the list view to ensure the new asset is added and sorted properly
  UpdateListView();

  // Find the list view item that was created for the asset, select it, and
  // start editing its text
  AssetBrowserListViewItem* lvitem = FindListViewItem(file);
  if(lvitem) {
    list_view_->UnselectAll();
    list_view_->SetFocus();
    lvitem->SetSelected(true);
    lvitem->BringIntoView();
    lvitem->BeginTextEdit();
  }
}

void AssetBrowser::OnKeyDown (SystemEvent* e) {
  switch(e->GetKeyCode()) {
    case Keys::F2: {
      Node* focus = GetWindow()->GetFocus();
      if(focus && (focus==list_view_ || focus->IsChildOf(list_view_))) {
        ListViewItem* lvitem = list_view_->GetFocusItem();
        if(lvitem) {
          lvitem->SetFocus();
          lvitem->BringIntoView();
          lvitem->BeginTextEdit();
        }
      }
      break;
    }

    case Keys::D: if(e->IsControl()) DuplicateSelectedItems (); break;
    case Keys::Delete: DeleteSelectedItems (); break;
  }

  Control::OnKeyDown(e);
}

bool AssetBrowser::GetSelectedItems (std::vector<AssetFile*>& files, std::vector<AssetFolder*>& folders) {
  // Determine which window has focus..
  Node* focus = GetWindow()->GetFocus();
  if(nullptr == focus) return false;

  // Make room in files and folders vector for all possible selected items.
  files.reserve(list_view_->GetSelectedItems().size() + tree_view_->GetSelectedItems().size());
  folders.reserve(files.capacity());

  if(focus==list_view_ || focus->IsChildOf(list_view_)) {
    for(noz_uint32 i=0,c=list_view_->GetSelectedItems().size(); i<c; i++) {
      AssetBrowserListViewItem* lvitem = (AssetBrowserListViewItem*)(ListViewItem*)list_view_->GetSelectedItems()[i];
      if(lvitem->folder_) folders.push_back(lvitem->folder_); 
      if(lvitem->file_) files.push_back(lvitem->file_);
    }
  } else if (focus==tree_view_ || focus->IsChildOf(tree_view_)) {
    for(noz_uint32 i=0,c=tree_view_->GetSelectedItems().size(); i<c; i++) {
      AssetBrowserTreeViewItem* tvitem = (AssetBrowserTreeViewItem*)(TreeViewItem*)tree_view_->GetSelectedItems()[i];
      folders.push_back(tvitem->folder_);
    }
  }  

  // If a selected file is within a selected folder then remove the file
  // from the list.
  if(folders.size()) {
    for(auto it=files.begin(); it!=files.end(); ) {
      bool contains = false;
      for(noz_uint32 i=0,c=folders.size();!contains&&i<c;i++) {
        contains = folders[i]->Contains(*it);
      }
      if(contains) {
        files.erase(it);
      } else {
        it++;
      }
    }
  }

  return (folders.size()>0 || files.size()>0);
}

void AssetBrowser::DuplicateSelectedItems (void) {
  // Retrieve the lists of selected files and folders
  std::vector<AssetFile*> files;
  std::vector<AssetFolder*> folders;
  if(!GetSelectedItems(files,folders)) return;

  // Prepare arrays to store the list of duplicated files
  std::vector<AssetFile*> duplicated_files;
  std::vector<AssetFolder*> duplicated_folders;
  duplicated_files.reserve(files.size());
  duplicated_folders.reserve(folders.size());

  for(noz_uint32 i=0,c=files.size();i<c;i++) {
    AssetFile* duplicated = AssetDatabase::DuplicateFile(files[i]);
    if(duplicated) duplicated_files.push_back(duplicated);
  }

  for(noz_uint32 i=0,c=folders.size();i<c;i++) {
  }

  UpdateListView();

  for(noz_uint32 i=0,c=duplicated_files.size();i<c;i++) {
    AssetFile* duplicated = duplicated_files[i];
    AssetBrowserListViewItem* item = FindListViewItem(duplicated);
    if(item) item->SetSelected(true);
  }
}

void AssetBrowser::DeleteSelectedItems (void) {
  std::vector<AssetFile*> files;
  std::vector<AssetFolder*> folders;
  if(!GetSelectedItems(files,folders)) return;

  // Warn for delete 
  MessageBoxResult result = MessageBox::Show(GetWindow(), "Are you sure you want to delete the selected items?\n\nYou cannot undo this action.", "Delete Selected Folders?", MessageBoxButton::YesNo, MessageBoxImage::Question);
  if(result == MessageBoxResult::No) return;

  // Delete all files first
  for(noz_uint32 i=0,c=files.size(); i<c; i++) {
    AssetDatabase::DeleteFile(files[i]);
  }

  // Delete all folders
  for(noz_uint32 i=0,c=folders.size(); i<c; i++) {
    AssetBrowserTreeViewItem* tvitem = FindTreeViewItem(folders[i],nullptr);
    if(AssetDatabase::DeleteFolder(folders[i])) {
      tvitem->Destroy();
    }
  }

  // Update the list view to reflect the changes
  UpdateListView();
}

#if 0
AssetFile* AssetBrowser::HandleListViewDragDrop(Object* o) {
  if(o==nullptr) return;

  AssetFolder* folder = tree_view_->GetFocusedItem();

  // Collection of objects?
  if(o->IsTypeOf(typeof(DragDropObjects))) {
    DragDropObjects* objects = (DragDropObjects*)o;
    for(noz_uint32 i=0,c=objects->GetCount();i<c;i++) {
      Object* o = (*objects)[i];
      AssetFile* file = CreateFileFromObject(o, folder);
    }
  }
}
#endif
///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "AssetEditor.h"

using namespace noz;
using namespace noz::Editor;


AssetEditor::AssetEditor(void) {
  file_ = nullptr;
}

AssetEditor::~AssetEditor(void) {
}

bool AssetEditor::Open (AssetFile* file) {
  file_ = file;
  if(!Load(file)) {
    file_ = nullptr;
    return false;
  }
  AssetDatabase::AssetRenamed += AssetRenamedEvent::Delegate(this, &AssetEditor::OnAssetRenamed);

  return true;
}

void AssetEditor::OnAssetRenamed (AssetFile* file) {
  if(file_ != file) return;
  SetName(file_->GetName());
  UpdateTitle();
}

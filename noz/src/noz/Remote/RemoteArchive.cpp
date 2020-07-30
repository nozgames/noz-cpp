///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "RemoteArchive.h"
#include "RemoteClientView.h"

using namespace noz;

RemoteArchive::RemoteArchive(RemoteClientView* view) : view_(view) {
}

Stream* RemoteArchive::OpenFile (const Guid& guid) {
  if(nullptr==view_) return nullptr;
  return view_->OpenAssetFile (guid);
}


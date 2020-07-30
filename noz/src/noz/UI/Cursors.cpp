///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Cursors.h"
#include "SystemCursor.h"

using namespace noz;


Cursors* Cursors::this_ = nullptr;
ObjectPtr<Cursor> Cursors::Default;
ObjectPtr<Cursor> Cursors::Cross;
ObjectPtr<Cursor> Cursors::SizeWE;
ObjectPtr<Cursor> Cursors::SizeNS;
ObjectPtr<Cursor> Cursors::SizeNWSE;
ObjectPtr<Cursor> Cursors::SizeNESW;
ObjectPtr<Cursor> Cursors::SizeAll;
ObjectPtr<Cursor> Cursors::IBeam;
ObjectPtr<Cursor> Cursors::Hand;

Cursors::Cursors(void) {
  Default = nullptr;
}


void Cursors::Initialize(void) {
  if(this_!=nullptr) return;

  this_ = new Cursors;

  this_->Default = new SystemCursor (SystemCursorType::Default);
  this_->Cross = new SystemCursor (SystemCursorType::Cross);
  this_->SizeWE = new SystemCursor (SystemCursorType::SizeWE);
  this_->SizeNS = new SystemCursor (SystemCursorType::SizeNS);
  this_->SizeNWSE = new SystemCursor (SystemCursorType::SizeNWSE);
  this_->SizeNESW = new SystemCursor (SystemCursorType::SizeNESW);
  this_->SizeAll = new SystemCursor (SystemCursorType::SizeAll);
  this_->IBeam = new SystemCursor (SystemCursorType::IBeam);
  this_->Hand = new SystemCursor (SystemCursorType::Hand);

  AssetManager::AddAsset(Guid::Parse("{223D005A-1A1C-4F6A-BB06-1E9C189614B5}"), this_->Default);
  AssetManager::AddAsset(Guid::Parse("{38561D96-D8DC-474B-9832-E40F02AC968E}"), this_->Cross);
  AssetManager::AddAsset(Guid::Parse("{7C2CBE6A-CABC-4238-9BC9-392F0945CC06}"), this_->SizeWE);
  AssetManager::AddAsset(Guid::Parse("{AB871075-A373-4B8D-97D7-61CA08496638}"), this_->SizeNS);
  AssetManager::AddAsset(Guid::Parse("{458246DD-C6CD-46ED-9051-6F3399326235}"), this_->SizeNESW);
  AssetManager::AddAsset(Guid::Parse("{87ED70E2-5CFF-4444-9F7F-7797D1718770}"), this_->SizeNWSE);
  AssetManager::AddAsset(Guid::Parse("{0B7DCE98-7E9F-433C-A68C-4E6ED5B26CA1}"), this_->SizeAll);
  AssetManager::AddAsset(Guid::Parse("{8FCFDF8F-E5E1-4067-8124-F7CE5E3948D3}"), this_->IBeam);
  AssetManager::AddAsset(Guid::Parse("{B9FB267C-674E-47EB-9A1C-1CE76C2E9C0C}"), this_->Hand);
}

void Cursors::Uninitialize(void) {
  if(this_==nullptr) return;

  delete this_;
  this_ = nullptr;
}



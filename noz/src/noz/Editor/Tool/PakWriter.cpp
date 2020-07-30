///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/BinarySerializer.h>
#include <noz/Assets/PakAssetArchive.h>
#include "PakWriter.h"

using namespace noz;
using namespace noz::Editor;


PakWriter::PakWriter (void) {
}

PakWriter::~PakWriter (void) {
}

bool PakWriter::Open (const String& path) {
  if(!stream_.Open(path, FileMode::Truncate)) {
    return false;
  }

  noz_uint32 fourcc = PakAssetArchive::FourCC;
  stream_.Write((char*)&fourcc, 0, sizeof(noz_uint32));
 
  // Placeholder for record count
  noz_uint32 placeholder = 0;
  stream_.Write((char*)&placeholder, 0, sizeof(placeholder));

  // Placeholder for record table offset
  stream_.Write((char*)&placeholder, 0, sizeof(placeholder));

  return true;
}

void PakWriter::Close (void) {
  noz_uint32 offset = stream_.GetPosition();

  stream_.Seek(sizeof(noz_uint32), SeekOrigin::Begin);

  noz_uint32 count = (noz_uint32)records_.size();
  stream_.Write((char*)&count, 0, sizeof(count));
  stream_.Write((char*)&offset, 0, sizeof(offset));  
  stream_.Seek(offset, SeekOrigin::Begin);
  stream_.Write((char*)&records_[0], 0, sizeof(PakAssetArchive::FileRecord) * count);

  stream_.Close();
}

bool PakWriter::AddAsset (AssetFile* file) {
  Asset* asset = file->Import();
  if(nullptr == asset) return false;

  PakAssetArchive::FileRecord record;
  record.offset_ = stream_.GetPosition();

  BinarySerializer().Serialize(asset,&stream_);
  delete asset;

  record.size_ = stream_.GetPosition() - record.offset_;
  record.guid_ = file->GetGuid();
  record.name_offset_ = 0;
  records_.push_back(record);

  return true;
}


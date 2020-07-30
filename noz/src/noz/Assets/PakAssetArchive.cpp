///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "PakAssetArchive.h"

using namespace noz;

const noz_uint32 PakAssetArchive::FourCC = 0x505A4F4E; // NOZP

PakAssetArchive::PakAssetArchive(const String& path) {
  path_ = path;

  if(!Open()) {
    stream_.Close();
  }
}

Stream* PakAssetArchive::OpenFile (const Guid& guid) {
  for(noz_uint32 i=0,c=files_.size();i<c;i++) {
    if(files_[i].guid_ == guid) {
      MemoryStream* stream = new MemoryStream(files_[i].size_);
      stream->SetLength(files_[i].size_);
      stream_.Seek(files_[i].offset_, SeekOrigin::Begin);
      stream_.Read((char*)stream->GetBuffer(),0,files_[i].size_);
      return stream;
    }
  }

  // Attempt to open the file
  return nullptr;
}

bool PakAssetArchive::Open(void) {
  // Open the stream
  if(!stream_.Open(path_, FileMode::Open )) return false;

  // Check four CC code
  noz_uint32 fourcc = 0;
  stream_.Read((char*)&fourcc, 0, 4);
  if(fourcc != FourCC) return false;

  // Read number of file records
  noz_uint32 file_count = 0;
  stream_.Read((char*)&file_count, 0, sizeof(noz_uint32));
  files_.resize(file_count);

  // Read offset of file records
  noz_uint32 file_offset = 0;
  stream_.Read((char*)&file_offset, 0, sizeof(noz_uint32));
  stream_.Seek(file_offset, SeekOrigin::Begin);

  // Read file records
  stream_.Read((char*)&files_[0],0,sizeof(FileRecord) * file_count);

  return true;
}

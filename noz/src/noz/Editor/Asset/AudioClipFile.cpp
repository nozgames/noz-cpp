///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/BinaryReader.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Serialization/JsonDeserializer.h>
#include "AudioClipFile.h"

using namespace noz;
using namespace noz::Editor;

AudioClipDef::AudioClipDef(void) {
}

AudioClipDef::~AudioClipDef(void) {
}

Asset* AudioClipFile::Import (void) {
  AudioClipDef def;

  FileStream fs;
  if(fs.Open(String::Format("%s.nozmeta",GetPath().ToCString()),FileMode::Open,FileAccess::Read)) {
    JsonDeserializer().Deserialize(&fs,&def);
    fs.Close();
  }

  if(!fs.Open(GetPath(),FileMode::Open,FileAccess::Read)) {
    return nullptr;
  }

  return Import(&fs, def);
}

AudioClip* noz::Editor::AudioClipFile::Import(Stream* stream, AudioClipDef& def) {
  struct WAVFormat { 
    noz_uint16 type_; 
    noz_uint16 channels_; 
    noz_uint32 samples_per_sec_; 
    noz_uint32 avg_bytes_per_sec_; 
    noz_uint16 block_align_;
    noz_uint16 bits_per_sample_;     
  } ;

  // Seek past 'RIFF' and 'WAVE'
  stream->Seek(12,SeekOrigin::Begin);

  // Read the block type
  noz_byte block_id[4];
  noz_uint32 block_size;
  noz_uint32 data_size = 0;
  noz_uint32 data_position = 0;
  WAVFormat data_format;
  memset(&data_format, 0,sizeof(data_format));
  
  BinaryReader reader(stream);

  // Read header blocks..
  while (stream->Read ((char*)block_id, 0, 4) == 4) {
    // Read the size of the block
    block_size=reader.ReadUInt32();

    // Next block position
    noz_uint32 next = noz_uint32(stream->GetPosition() + block_size);
  
    // Data block?
    if (block_id[0] == 'd' ) {
      data_size = block_size;
      data_position = noz_uint32(stream->GetPosition());

    // Fmt block?
    } else if (block_id[0]=='f' && block_id[1]=='m') {
      stream->Read ((char*)&data_format, 0, sizeof(WAVFormat) );      

    // Fact block?
    } else if (block_id[0]=='f' && block_id[1]=='a') {

    // LIST block?
    } else if (block_id[0]=='L' && block_id[1]=='I') {
    // Unknown block
    } 
    
    // Seek to next block
    stream->Seek(next,SeekOrigin::Begin);
  }

  // Ensure data was found.
  if(data_size==0 || data_position==0) return nullptr;

  // Ensure the format is recognizable 
  if(data_format.type_ != 1 || data_format.bits_per_sample_ != 16) return nullptr;

  // Read the data
  noz_byte* data = new noz_byte[data_size];
  stream->Seek(data_position, SeekOrigin::Begin);
  stream->Read((char*)data, 0, data_size);

  // Create the new clip
  AudioClip* clip = new AudioClip(data, data_size, data_format.samples_per_sec_, data_format.bits_per_sample_, data_format.channels_==1);
  delete[] data;

  return clip;
}

DateTime AudioClipFile::GetModifiedDate(void) const {
  DateTime d1 = File::GetLastWriteTime(GetPath());
  DateTime d2 = File::GetLastWriteTime(String::Format("%s.nozmeta",GetPath().ToCString()));

  if(d2 > d1) return d2;
  return d1;
}



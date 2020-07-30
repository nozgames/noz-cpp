///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Serialization/JsonDeserializer.h>
#include "ImageFile.h"
#include <noz/Render/Imaging/SignedDistanceFieldFilter.h>
#include <noz/Render/Imaging/ResizeFilter.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#define STBI_ONLY_PNG
#include <external/stb/stb_image.h>
#include <external/stb/stb_image_write.h>

using namespace noz;
using namespace noz::Editor;

ImageDef::ImageDef(void) {
  wrap_mode_ = ImageWrapMode::Clamp;
  filter_mode_ = ImageFilterMode::None;
  resize_width_ = 0;
  resize_height_ = 0;
  sdf_ = false;
}

ImageDef::~ImageDef(void) {
}


Asset* ImageFile::Import (void) {
  ImageDef meta;

  FileStream fs;
  if(fs.Open(String::Format("%s.nozimage",GetPath().ToCString()),FileMode::Open,FileAccess::Read)) {
    JsonDeserializer().Deserialize(&fs,&meta);
    fs.Close();
  }

  if(!fs.Open(GetPath(),FileMode::Open,FileAccess::Read)) {
    return nullptr;
  }

  return Import(&fs, meta);
}

Image* noz::Editor::ImageFile::Import(Stream* stream, ImageDef& def) {
  struct Local {
    Stream* stream;
    noz_uint32 length;
    noz_uint32 position;

    static int STBRead (void* user, char* data, int size) {
      Local* local = (Local*)user;
      noz_uint32 read = local->stream->Read(data, 0, size);
      local->position += read;
      return read;
    }

    static void STBSkip (void* user, int n) {
      Local* local = (Local*)user;
      local->stream->Seek(n,SeekOrigin::Current);
      local->position += n;
    }

    static int STBEOF (void* user) {
      Local* local = (Local*)user;
      return local->position >= local->length;
    }
  };

  Local local;
  local.length = stream->GetLength() - stream->GetPosition();;
  local.position = 0;
  local.stream = stream;

  stbi_io_callbacks callbacks;
  callbacks.eof = &Local::STBEOF;
  callbacks.read = &Local::STBRead;
  callbacks.skip = &Local::STBSkip;

  int x;
  int y;
  int n;
  unsigned char* data = stbi_load_from_callbacks(&callbacks, &local, &x, &y, &n, 0);
  if(nullptr==data) {
    return nullptr;
  }

  ImageFormat format = ImageFormat::Unknown;
  switch(n) {
    case 1: format = ImageFormat::A8; break;
    case 3: format = ImageFormat::R8G8B8; break;
    case 4: format = ImageFormat::R8G8B8A8; break;
    default:
      stbi_image_free(data);
      return nullptr;
  }

  Image* image = new Image(x,y, format);  
  memcpy(image->Lock(), data, n * x * y);
  image->SetWrapMode(def.wrap_mode_);
  image->SetFilterMode(def.filter_mode_);
  image->Unlock();

  stbi_image_free(data);

  if(def.sdf_) {
    SignedDistanceFieldFilter filter;
    Image* filtered = filter.Filter(image);
    delete image;
    image = filtered;
  }

  if(def.resize_width_ > 0 && def.resize_height_ > 0) {
    ResizeFilter resize_filter;
    resize_filter.SetWidth(def.resize_width_);
    resize_filter.SetHeight(def.resize_height_);
    Image* filtered = resize_filter.Filter(image);
    delete image;
    image = filtered;
  }

  return image;
}

void Image::Save (Stream* stream) {
  struct Local {
    Stream* stream;

    static void STBWrite (void* user, void* data, int size) {
      Local* local = (Local*)user;
      local->stream->Write((char*)data, 0, size);
    }
  };

  // Lock the image to ensure the buffer is populated
  noz_byte* buffer = Lock();

  Local local;
  local.stream = stream;
  stbi_write_png_to_func(
    &Local::STBWrite,
    &local, GetWidth(),
    GetHeight(),
    GetDepth(),
    buffer,
    GetStride()
  );

  Unlock();
}

Node* ImageFile::CreateNode (void) const {
  Image* image = AssetManager::LoadAsset<Image>(GetGuid());
  if(nullptr == image) {
    return nullptr;
  }

  ImageNode* node = new ImageNode;
  node->SetImage(image);
  return node;
}


DateTime ImageFile::GetModifiedDate(void) const {
  DateTime d1 = File::GetLastWriteTime(GetPath());
  DateTime d2 = File::GetLastWriteTime(String::Format("%s.nozimage",GetPath().ToCString()));

  if(d2 > d1) return d2;
  return d1;
}


bool ImageFile::Reimport (Asset* asset) {
  Image* image = Cast<Image>(asset);
  if(nullptr == image) return false;

  ImageDef meta;

  FileStream fs;
  if(fs.Open(String::Format("%s.nozimage",GetPath().ToCString()),FileMode::Open,FileAccess::Read)) {
    JsonDeserializer().Deserialize(&fs,&meta);
    fs.Close();
  }

  if(!fs.Open(GetPath(),FileMode::Open,FileAccess::Read)) {
    return false;
  }

  Image* new_image = Import(&fs, meta);
  image->SetFrom(new_image);
  delete new_image;
  return true;
}
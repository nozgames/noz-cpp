///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonSerializer.h>
#include "ImageImporter.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <external/stb/stb_image.h>

using namespace noz;

ImageMeta::ImageMeta(void) {
}

ImageMeta::~ImageMeta(void) {
  for(auto it_filter=filters_.begin(); it_filter!=filters_.end(); it_filter++) {
    delete (*it_filter);
  }
  filters_.clear();
}


Asset* ImageImporter::Import (const String& path) {
  FileStream fs;
  if(!fs.Open(path,FileMode::Open,FileAccess::Read)) {
    return nullptr;
  }

  return Import(&fs);
}

Image* noz::ImageImporter::Import(Stream* stream) {
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
  memcpy(image->GetBuffer(), data, n * x * y);
  stbi_image_free(data);

  return image;
}

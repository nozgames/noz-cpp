///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/StreamReader.h>
#include <noz/Serialization/JsonDeserializer.h>

#include "TextImporter.h"

using namespace noz;

Asset* TextImporter::Import (const String& path) {
  static const Name MetaImportType("ImportType");

  // Find the import type for the current importer
  Type* import_type = Type::FindType(GetType()->GetMeta(MetaImportType));
  if(nullptr==import_type) {
    return nullptr;
  }

  // Open the import file stream
  FileStream fs;
  if(!fs.Open(path, FileMode::Open)) {
    return nullptr;
  }

  TextType tt = GetTextType(&fs);
  Object* o = nullptr;
  switch(tt) {
    case TextType::Unknown: 
      // TODO: report error
      return nullptr;

    case TextType::Json: {
      o = (Object*)JsonDeserializer().Deserialize(&fs);
      break;
    }
  }

  // Validate the type
  if(!o->IsType(import_type)) {
    delete o;
    return nullptr;
  }

  return (Asset*)o;
}


TextImporter::TextType TextImporter::GetTextType (Stream* stream) {
  // Determine the type...
  StreamReader reader(stream);

  char c;
  while(0 != (c=reader.Read())) {
    if(c != '\r' && c != '\n' && c!='\t' && c!=' ') break;
  }

  stream->Seek(0,SeekOrigin::Begin);

  switch(c) {
    case '{': return TextType::Json;     
    default:
      break;
  }

  return TextType::Unknown;
}

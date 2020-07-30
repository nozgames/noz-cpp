///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonDeserializer.h>
#include <noz/Serialization/JsonSerializer.h>

#include "StyleSheetFile.h"

using namespace noz;
using namespace noz::Editor;

Asset* StyleSheetFile::Import (void) {
  // Open the import file stream
  FileStream fs;
  if(!fs.Open(GetPath(), FileMode::Open)) {
    return nullptr;
  }

  // Deserialize the object
  Object* o = (Object*)JsonDeserializer().Deserialize(&fs);

  // Close the file
  fs.Close();

  // Validate the type
  if(o && !o->IsType(typeof(StyleSheet))) {
    delete o;
    return nullptr;
  }

  return (Asset*)o;
}

bool StyleSheetFile::CreateEmpty (void) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Truncate)) return nullptr;

  StyleSheet ss;
  JsonSerializer().Serialize(&ss, &fs);
  fs.Close();
  
  return true;
}
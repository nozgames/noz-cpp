///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonDeserializer.h>
#include <noz/Serialization/JsonSerializer.h>

#include "SceneFile.h"

using namespace noz;
using namespace noz::Editor;

Asset* SceneFile::Import (void) {
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
  if(o && !o->IsType(typeof(Scene))) {
    delete o;
    return nullptr;
  }

  return (Asset*)o;
}

bool SceneFile::CreateEmpty (void) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Truncate)) return nullptr;

  Scene scene;
  JsonSerializer().Serialize(&scene, &fs);
  fs.Close();
  
  return true;
}
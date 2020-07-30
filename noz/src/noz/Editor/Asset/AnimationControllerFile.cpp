///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Animation/AnimationController.h>
#include <noz/Serialization/JsonDeserializer.h>
#include <noz/Serialization/JsonSerializer.h>

#include "AnimationControllerFile.h"

using namespace noz;
using namespace noz::Editor;

bool AnimationControllerFile::CreateEmpty (void) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Truncate)) return nullptr;

  AnimationController sprite;
  JsonSerializer().Serialize(&sprite, &fs);
  fs.Close();
  
  return true;
}

Asset* AnimationControllerFile::Import (void) {
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
  if(o && !o->IsType(typeof(AnimationController))) {
    delete o;
    return nullptr;
  }

  return (Asset*)o;
}

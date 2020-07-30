///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonDeserializer.h>
#include <noz/Serialization/JsonSerializer.h>

#include "SpriteFile.h"

using namespace noz;
using namespace noz::Editor;

bool SpriteFile::CreateEmpty (void) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Truncate)) return nullptr;

  Sprite sprite;
  JsonSerializer().Serialize(&sprite, &fs);
  fs.Close();
  
  return true;
}

Asset* SpriteFile::Import (void) {
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
  if(o && !o->IsType(typeof(Sprite))) {
    delete o;
    return nullptr;
  }

  return (Asset*)o;
}

Node* SpriteFile::CreateNode (void) const {
  Sprite* sprite = AssetManager::LoadAsset<Sprite>(GetGuid());
  if(nullptr == sprite) {
    return nullptr;
  }

  SpriteNode* node = new SpriteNode;
  node->SetSprite(sprite);
  return node;
}

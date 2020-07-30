///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Serialization/JsonDeserializer.h>
#include <noz/Serialization/JsonSerializer.h>
#include <noz/Serialization/BinarySerializer.h>
#include "StyleFile.h"

using namespace noz;
using namespace noz::Editor;

bool StyleFile::CreateEmpty (void) {
  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Truncate)) return nullptr;

  Style::Def def;
  JsonSerializer().Serialize(&def, &fs);
  fs.Close();
  
  return true;
}

Asset* StyleFile::Import (void) {
  Style* style = new Style;
  if(!Reimport(style)) {
    delete style;
    return nullptr;
  }

  return style;
}

bool StyleFile::Reimport (Asset* asset) {
  Style* style = Cast<Style>(asset);
  if(nullptr==style) return false;

  // Open the style definition file.
  FileStream fs;
  if(!fs.Open(GetPath(),FileMode::Open)) return nullptr;

  // Deserialize the stream into a StyleDef objestyle
  Style::Def def;
  if(nullptr==JsonDeserializer().Deserialize(&fs,&def)) {
    return nullptr;
  }

  // Close the input file
  fs.Close();

  // Create the style
  style->control_type_ = def.control_type_;

  // Create a style instance and serialize into the style instance object
  Style::Template t;
  t.parts_ = def.parts_;
  t.nodes_ = def.nodes_;
  style->template_.Set(&t);

  NOZ_TODO("handle nullptr on control type");

  // Return the style
  return true;
}

Node* StyleFile::CreateNode (void) const {
  Style* style = AssetManager::LoadAsset<Style>(GetGuid());
  if(nullptr == style) {
    return nullptr;
  }

  Control* control = style->GetControlType()->CreateInstance<Control>();
  if(nullptr == control) {
    return nullptr;
  }

  control->SetStyle(style);
  return control;
}

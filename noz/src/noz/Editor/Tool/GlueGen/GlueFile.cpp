///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "GlueGen.h"

using namespace noz;
using namespace noz::Editor;

GlueFile::GlueFile(void) {
  excluded_ = false;
  type_ = GlueFileType::Unknown;
  glue_ = true;
}

void GlueFile::SetFullPath (const String& path) {
  full_path_ = path;
  directory_ = Path::GetDirectoryName(full_path_);
  type_ = GetGlueFileType(path);
}

void GlueFile::SetPath (const String& path) {
  path_ = path;
}

void GlueFile::SetExcluded (bool excluded) {
  excluded_ = excluded;
}

bool GlueFile::Load (void) {
  // Open the file stream.
  FileStream fs;
  if(!fs.Open(full_path_,FileMode::Open)) {
    Console::WriteLine ("%s: error: file could not be opened", full_path_.ToCString());
    return false;
  }

  // Read entire file contents into memory
  noz_uint32 size = fs.GetLength();
  data_.resize(size+1);
  size = fs.Read(&data_[0],0,size);
  data_[size] = 0;
  fs.Close();

  Preprocess();

  // Load the file contents into the lexer
  return true;
  //return lexer_.load(full_path_.ToCString());
}

void GlueFile::Preprocess(void) {
#if 0
  StringBuilder sb;
  
  for(const char* s = &data_[0]; *s; s++) {
    const char* e;
    for(e=s; *e && *e != '#'; e++);
    if(e != s) {
      sb.Append(s,e-s);
    }

    if(*e=='#') {
      
    }
  }
#endif
}


GlueLexer* GlueFile::CreateLexer (void) {  
  return new GlueLexer(&data_[0],data_.size());
}

GlueFileType GlueFile::GetGlueFileType (const String& path) {
  // Determine the file type.
  static const String EXT_H (".h");
  static const String EXT_CPP (".cpp");
  static const String EXT_C (".c");
  static const String EXT_PCH (".pch");

  GlueFileType type = GlueFileType::Unknown;

  String ext = Path::GetExtension(path);
  if(!ext.CompareTo(EXT_H)) {
    type = GlueFileType::H;
  } else if(!ext.CompareTo(EXT_PCH)) {    
    type = GlueFileType::PCH;
  } else if(!ext.CompareTo(EXT_C)) {
    type = GlueFileType::C;
  } else if(!ext.CompareTo(EXT_CPP)) {
    type = GlueFileType::CPP;
  } else {
    type = GlueFileType::Unknown;
  }

  return type;
}

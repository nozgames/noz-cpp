///////////////////////////////////////////////////////////////////////////////
// noZ Glue Compiler
// Copyright (C) 2013-2014 Bryan Dube / Radius Software
// http://www.radius-software.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <cstdlib>
#include <noz/Editor/Tool/Makefile.h>
#include <noz/Text/Json/JsonObject.h>
#include <noz/Text/Json/JsonArray.h>
#include <noz/Text/Json/JsonString.h>

#include "GlueGen.h"

using namespace noz;
using namespace noz::Editor;


GlueState::GlueState (void) {
  current_path_ = Environment::GetCurrentDirectory();
  clean_ = false;
  precompiled_header_ = nullptr;
}


GlueEnum* GlueState::AddEnum(const Name& ns, const Name& name) {
  Name qualified_name = String::Format("%s::%s", ns.ToCString(), name.ToCString());

  GlueEnum* ge = GetQualifiedEnum(qualified_name);
  if(ge) return ge;

  // Allocate a new enum
  ge = new GlueEnum;
  ge->SetNamespace(ns);;
  ge->SetQualifiedName(qualified_name);
  ge->SetName(name);
  enums_.push_back(ge);
  enums_by_qualified_name_[qualified_name] = ge;
  return ge;
}

GlueClass* GlueState::AddClass(GlueClass* gc) {
  classes_.push_back(gc);
  classes_by_name_[gc->GetName()] = gc;
  classes_by_qualified_name_[gc->GetQualifiedName()] = gc;
  return gc;
}

GlueClass* GlueState::AddClass(const Name& ns, const Name& name) {
  Name qualified_name;
  if(ns.IsEmpty()) {
    qualified_name = name;
  } else {
    StringBuilder sb;  
    sb.Append(ns);
    sb.Append("::");
    sb.Append(name);
    qualified_name = sb.ToString();
  }
  
  GlueClass* gc = GetQualifiedClass(qualified_name);
  if(gc) {
    return gc;
  }

  gc = new GlueClass;
  gc->SetNamespace(ns);
  gc->SetName(name);;
  gc->SetQualifiedName(qualified_name);
  return AddClass(gc);
  return gc;
}

int GlueState::GlueClassSort(const void* _gc1, const void* _gc2) {
  return (*((GlueClass**)_gc2))->GetSortKey() - (*((GlueClass**)_gc1))->GetSortKey();
}

void GlueState::AddIncludeDirectory (const Name& dir) {
  include_directories_.insert(dir);
}

bool GlueState::AddFile(GlueFile* gf) {
  if(gf->GetFullPath().Equals(output_path_,StringComparison::OrdinalIgnoreCase)) {
    delete gf;
    return true;
  }

  // Attempt to load the file
  if(!gf->Load ()) return false;

  // Cache precompiled header.
  if(gf->IsPrecompiledHeader()) {
    precompiled_header_ = gf;
  }

  // Add to file vector
  files_.push_back(gf);

  // Add the file to the full path map
  files_by_full_path_[gf->GetFullPath().Lower()] = gf;

  return true;
}

GlueFile* GlueState::GetFile (const Name& full_path) const {  
  auto it = files_by_full_path_.find(String(full_path).Lower());
  if(it==files_by_full_path_.end()) return nullptr;
  return it->second;
}

bool GlueState::IsBuildRequired (void) const {
  noz_assert(!output_path_.IsEmpty());

  // Alwasy build if clean specified.
  if(clean_) return true;

  // Get modified time of executable.
  DateTime exe_time = File::GetLastWriteTime(Environment::GetExecutablePath());

  // Get modified time of target
  DateTime target_time = File::GetLastWriteTime(output_path_);

  // If the executable is newer than the target then rebuild 
  if(exe_time.CompareTo(target_time) > 0) {
    return true;
  }

  // Scan all of the header files and see if any of them are newer than
  // the target file
  for(auto it=GetFiles().begin(); it!=GetFiles().end(); it++) {
    GlueFile* gf = *it;
    noz_assert(gf);

    // Only care about included files
    if(gf->IsExcluded()) continue;

    // Only care about header files.
    if(gf->GetType() != GlueFileType::H) continue;

    // If the header file time is greater than the target time then rebuild
    DateTime file_time = File::GetLastWriteTime(gf->GetFullPath());
    if(file_time.CompareTo(target_time) > 0) {
      return true;
    }
  }

  return false;
}

bool GlueState::ProcessClasses (void) {
  // Resolve base classes
  for(size_t it=0; it<classes_.size(); it++) 
    if(!classes_[it]->ResolveBaseClasses(this))
      return false;

  // Resolve base classes
  for(size_t it=0; it<classes_.size(); it++) 
    if(!classes_[it]->Process(this))
      return false;

  // Sort the classes.
  std::qsort(&classes_[0],classes_.size(),sizeof(GlueClass*),GlueClassSort);

  return true;
}

GlueClass* GlueState::GetClass (const Name& name) const {
  auto it = classes_by_name_.find (name);
  if(it != classes_by_name_.end()) return it->second;
  return nullptr;
}

GlueClass* GlueState::GetQualifiedClass (const Name& qualifed_name) const {
  auto it = classes_by_qualified_name_.find (qualifed_name);
  if(it != classes_by_qualified_name_.end()) return it->second;
  return nullptr;
}

GlueEnum* GlueState::GetQualifiedEnum (const Name& qualifed_name) const {
  auto it = enums_by_qualified_name_.find (qualifed_name);
  if(it != enums_by_qualified_name_.end()) return it->second;
  return nullptr;
}

GlueType* GlueState::GetQualifiedType (const Name& qualifed_name) {
  return &types_by_qualified_name_[qualifed_name];
}

bool GlueState::LoadProject (void) {
  Makefile::ParseOptions options;
  options.defines.insert(defines_.begin(), defines_.end());
  options.platform = platform_;

  // Load the makefile to generate glue on
  Makefile* makefile = Makefile::Parse(makefile_path_, options);
  if(nullptr == makefile) {
    return false;
  }

  Makefile::Configuration* configuration = nullptr;;
  for(noz_uint32 i=0,c=makefile->configurations_.size(); i<c; i++) {
    if(makefile->configurations_[i]->name_.Equals(config_name_,StringComparison::OrdinalIgnoreCase)) {
      configuration = makefile->configurations_[i];
      break;
    }
  }

  if(nullptr == configuration) {
    Console::WriteLine("%s: error: configuration '%s' not found", makefile_path_.ToCString(), config_name_.ToCString());
    return false;
  }

  if(output_path_.IsEmpty()) {
    output_path_ = 
      Path::Combine(target_directory_.IsEmpty() ? makefile->target_dir_ : target_directory_, 
        Path::Combine(
          Path::Combine(configuration->output_directory_,"glue"),
          Path::ChangeExtension(makefile->name_, ".glue.h")
        )
      );
  }

  for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
    const Makefile::BuildFile& file = *makefile->files_[i];

    String ext = Path::GetExtension(file.path_);
    if(!ext.Equals(".cpp", StringComparison::OrdinalIgnoreCase) &&
       !ext.Equals(".c", StringComparison::OrdinalIgnoreCase) &&
       !ext.Equals(".h", StringComparison::OrdinalIgnoreCase)     ) {
      continue;
    }

    String full_path = Path::Canonical(Path::Combine(makefile->target_dir_, file.path_));

    // Add the include file to be processed.
    GlueFile* gf = new GlueFile;
    gf->SetPath(file.path_);
    gf->SetFullPath(full_path);
    gf->SetExcluded(false);
    gf->SetGlueFile(file.glue_);

    if(!AddFile(gf)) {
      return false;
    }
  }
 
  // Add include directories to the state
  for(noz_uint32 i=0,c=configuration->include_directories_.size(); i<c; i++) {
    AddIncludeDirectory(
      Path::Canonical(Path::Combine(makefile->target_dir_,configuration->include_directories_[i]))
    );
  }
  
  return true;
}


GlueClass* GlueState::ResolveClass (const Name& name, const Name& _ns) const {
  String ns = _ns;

  while(1) {
    GlueClass* gc = nullptr;

    // Look up the class..
    if(ns.IsEmpty()) {
      gc = GetQualifiedClass(name);
    } else {
      gc = GetQualifiedClass(String::Format("%s::%s", ns.ToCString(), name.ToCString()));
    }

    // Return the class if we found it..
    if(gc) return gc;

    // If the namespace is empty then we are done.
    if(ns.IsEmpty()) break;

    // Remove the top-most namespace
    int li = ns.LastIndexOf(':');
    if(li==-1) {
      ns = "";
    } else {
      ns = ns.Substring(0,li-1);
    }
  } 
      
  return nullptr;
}

GlueEnum* GlueState::ResolveEnum (const Name& name, const Name& _ns) const {
  String ns = _ns;

  while(1) {
    GlueEnum* ge = nullptr;

    // Look up the enum ..
    if(ns.IsEmpty()) {
      ge = GetQualifiedEnum(name);
    } else {
      ge = GetQualifiedEnum(String::Format("%s::%s", ns.ToCString(), name.ToCString()));
    }

    // Return the enum if we found it..
    if(ge) return ge;

    // If the namespace is empty then we are done.
    if(ns.IsEmpty()) break;

    // Remove the top-most namespace
    int li = ns.LastIndexOf(':');
    if(li==-1) {
      ns = "";
    } else {
      ns = ns.Substring(0,li-1);
    }
  } 
      
  return nullptr;
}

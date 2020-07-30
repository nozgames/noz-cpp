///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "ProjectGen.h"
#include "VisualStudioProjectGen.h"
#include "XCodeProjectGen.h"

using namespace noz;


noz_int32 ProjectGen::FindReference (const String& path) const {
  for(noz_uint32 i=0,c=references_.size(); i<c; i++) {
    if(references_[i]->path_.Equals(path)) return (noz_int32)i;
  }
  return -1;
}

Makefile* ProjectGen::GetReferenceMakefile (Makefile* makefile, const String& ref) const {
  String path = Path::Combine(makefile->path_,ref);
  path = Path::Canonical(path);

  noz_int32 i = FindReference(path);
  if(i==-1) return nullptr;

  return references_[i];
}

bool ProjectGen::LoadReferences(Makefile* makefile) {
  noz_uint32 rinsert = references_.size();

  for(noz_uint32 i=0,c=makefile->references_.size(); i<c; i++) {
    const Makefile::Reference& r = makefile->references_[i];

    String path = Path::Combine(makefile->path_,r.path_);
    path = Path::Canonical(path);

    // If the reference already exists just move it to the end of the references list
    noz_int32 ri = FindReference(path);
    if(ri != -1) {
      Makefile* rmakefile = references_[ri];
      references_.erase(references_.begin() + ri);
      references_.push_back(rmakefile);
      continue;
    }

    Makefile* rm = Makefile::Parse(path, makefile->options_);
    if(nullptr == rm) {
      Console::WriteLine("%s: error: failed to load reference file '%s'", makefile->path_.ToCString(), path.ToCString());
      return false;
    }

    if(rinsert >= references_.size()) {
      references_.push_back(rm);
    } else {
      references_.insert(references_.begin() + rinsert, rm);
    }
    rinsert++;

    if(!LoadReferences(rm)) return false;
  }

  return true;
}


bool ProjectGen::Generate (const String& makefile_path, const Makefile::ParseOptions& options) {
  Makefile* makefile = Makefile::Parse(makefile_path, options);
  if(nullptr == makefile) return false;

  bool result = Generate(makefile,String::Empty);
  delete makefile;
  return result;
}


bool ProjectGen::Generate (Makefile* makefile, const String& target_directory) {
  noz_assert(makefile);

  ProjectGen* gen = nullptr;
  switch(makefile->platform_) {
    case Makefile::PlatformType::Windows: gen = new VisualStudioProjectGen; break;
    case Makefile::PlatformType::IOS: gen = new XCodeProjectGen; break;
    case Makefile::PlatformType::OSX: gen = new XCodeProjectGen; break;
    default:
      delete makefile;
      return false;
  }

  gen->references_.push_back(makefile);
  if(!gen->LoadReferences(makefile)) {
    delete gen;
    return false;
  }

  String base_dir = Path::GetDirectoryName(Path::GetDirectoryName(makefile->path_));

  String target;
  if(target_directory.IsEmpty()) {
    target = makefile->target_dir_;
  } else {
    target = target_directory;
  }

  bool result = true;
  for(noz_uint32 i=gen->references_.size(); result && i>1; i--) {
    String ref_target = Path::GetRelativePath(gen->references_[i-1]->target_dir_,makefile->target_dir_);
    ref_target = Path::Canonical(Path::Combine(target,ref_target));
    result &= gen->GenerateOverride (gen->references_[i-1],ref_target);
  }

  if(result) result &= gen->GenerateOverride (gen->references_[0],target);

  delete gen;

  return result;
}

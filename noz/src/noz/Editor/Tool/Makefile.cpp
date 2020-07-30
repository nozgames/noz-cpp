///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Text/StringBuilder.h>
#include <noz/Text/Json/JsonReader.h>
#include <noz/IO/StringReader.h>
#include "Makefile.h"

using namespace noz;

Makefile* Makefile::Parse (const String& path, const ParseOptions& options) {
  FileStream fs;
  if(!fs.Open(path, FileMode::Open)) return nullptr;

  noz_uint32 length = fs.GetLength();

  noz_byte BOM[3];
  noz_int32 read = fs.Read((char*)BOM,0,3);
  if(3!=read||BOM[0]!=0xEF||BOM[1]!=0xBB||BOM[2]!=0xBF) {
    fs.Seek(-3,SeekOrigin::Current);
  } else {
    length -= 3;
  }
  
  char* text = new char[length+1];
  fs.Read(text, 0, length);
  text[length] = 0;

  String preprocessed = Preprocess(text, options);
  StringReader sreader (preprocessed);
  JsonReader reader (&sreader, path.ToCString());

  Makefile* makefile = new Makefile;
  makefile->options_ = options;
  makefile->path_ = path;
  makefile->name_ = Path::GetFilenameWithoutExtension(path);
  makefile->platform_ = options.platform;
  makefile->target_dir_ = Path::Canonical(Path::Combine(Path::GetDirectoryName(path),PlatformTypeToString(options.platform)));
  if(!Parse(makefile, reader)) {
    delete makefile;
    makefile = nullptr;
  }

  delete[] text;

  if(makefile) {
    for(noz_uint32 i=0,c=makefile->configurations_.size(); i<c; i++) {
      Configuration* cfg = makefile->configurations_[i];
      MergeConfiguration(&makefile->default_configuration_, cfg);

      // Resolve the output directory
      if(cfg->output_directory_.IsEmpty()) cfg->output_directory_ = "output";
      cfg->output_directory_ = Path::Combine(cfg->output_directory_,cfg->name_);
      cfg->output_directory_ = Path::Combine(cfg->output_directory_,makefile->name_);
      cfg->output_directory_ = Path::Canonical(cfg->output_directory_);

      // Generate the out dir
      if(cfg->bin_directory_.IsEmpty()) cfg->bin_directory_ = "bin";
      cfg->bin_directory_ = Path::Combine(cfg->bin_directory_,cfg->name_);
      cfg->bin_directory_ = Path::Canonical(cfg->bin_directory_);

      if(makefile->glue_) {
        cfg->include_directories_.push_back(Path::Combine(cfg->output_directory_, "glue"));        
      }
    }

    for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
      BuildFile* file = makefile->files_[i];
      if(!file->group_.IsEmpty()) continue;
      if(!makefile->auto_group_directory_.IsEmpty()) {
        String dir = file->directory_ ? file->path_ : Path::GetDirectoryName(file->path_);
        file->group_ = Path::GetRelativePath(Path::Canonical(Path::Combine(makefile->target_dir_,dir)),makefile->auto_group_directory_);
        if(!file->group_.IsEmpty() && file->group_[0]=='.') file->group_ = String::Empty;
      }
      if(!makefile->auto_group_base_.IsEmpty()) {
        file->group_ = Path::Combine(makefile->auto_group_base_, file->group_);
      }
    }
  }

  return makefile;
}

bool Makefile::Parse(Makefile* makefile, JsonReader& reader) {
  if(!reader.ReadStartObject()) return false;

  while(!reader.IsEOS() && !reader.IsEndObject()) {
    Name member;
    if(!reader.ReadMember(member)) return false;

    // TargetType
    static Name key_TargetType ("TargetType");
    if(member == key_TargetType) {
      String value;
      if(!reader.ReadValueString(value)) return false;
      if(value.Equals("Application")) {
        makefile->target_type_ = TargetType::Application;
        continue;
      }
      if(value.Equals("Library")) {
        makefile->target_type_ = TargetType::Library;
        continue;
      }
      if(value.Equals("Console")) {
        makefile->target_type_ = TargetType::Console;
        continue;
      }
      reader.ReportError(String::Format("unknown TargetType '%s'", value.ToCString()).ToCString());
      continue;
    }

    // Glue
    static Name key_Glue ("Glue");
    if(member == key_Glue) {
      if(!reader.ReadValueBoolean(makefile->glue_)) return false;
      continue;
    }

    // Organization
    static Name key_Organization ("Organization");
    if(member == key_Organization) {
      if(!reader.ReadValueString(makefile->organization_)) return false;
      continue;
    }

    // BundleId
    static Name key_BundleId ("BundleId");
    if(member == key_BundleId) {
      if(!reader.ReadValueString(makefile->bundle_id_)) return false;
      continue;
    }

    // AutoGroupDirectory
    static Name key_AutoGroupDirectory ("AutoGroupDirectory");
    if(member == key_AutoGroupDirectory) {
      if(!reader.ReadValueString(makefile->auto_group_directory_)) return false;
      makefile->auto_group_directory_ = Path::Canonical(Path::Combine(Path::GetDirectoryName(makefile->path_),makefile->auto_group_directory_));
      continue;
    }

    // AutoGroupBase
    static Name key_AutoGroupBase ("AutoGroupBase");
    if(member == key_AutoGroupBase) {
      if(!reader.ReadValueString(makefile->auto_group_base_)) return false;
      continue;
    }

    // Guid
    static Name key_Guid ("Guid");
    if(member == key_Guid) {
      String value;
      if(!reader.ReadValueString(value)) return false;
      makefile->guid_ = Guid::Parse(value);
      continue;
    }

    // SolutionGuid
    static Name key_SolutionGuid ("SolutionGuid");
    if(member == key_SolutionGuid) {
      String value;
      if(!reader.ReadValueString(value)) return false;
      makefile->solution_guid_ = Guid::Parse(value);
      continue;
    }

    // Files
    static Name key_Files ("Files");
    if(member == key_Files) {
      if(!ParseFiles(makefile,reader)) return false;
      continue;
    }

    // AssetDirectories
    static Name key_AssetDirectories ("AssetDirectories");
    if(member == key_AssetDirectories) {
      if(!reader.ReadStartArray()) return false;
      while(!reader.IsEndArray()) {
        String path;
        if(!reader.ReadValueString(path)) return false;
        path = Path::Canonical(Path::Combine(Path::GetDirectoryName(makefile->path_),path));
        path = Path::Canonical(Path::GetRelativePath(path,makefile->target_dir_));
        makefile->assets_directories_.push_back(path);
      }
      if(!reader.ReadEndArray()) return false;
      continue;
    }
    
    // References
    static Name key_References("References");
    if(member == key_References) {
      if(!reader.ReadStartArray()) return false;
      while(!reader.IsEndArray()) {
        String path;
        if(!reader.ReadValueString(path)) return false;
        path = Path::Canonical(Path::Combine(Path::GetDirectoryName(makefile->path_),path));
        path = Path::Canonical(Path::GetRelativePath(path,makefile->target_dir_));
        Reference r;
        r.path_ = path;
        makefile->references_.push_back(r);
      }
      if(!reader.ReadEndArray()) return false;
      continue;
    }

    // Configurations
    static Name key_Configurations ("Configurations");
    if(member == key_Configurations) {
      if(!ParseConfigurations(makefile,reader)) return false;
      continue;
    }

    // DefaultConfiguration
    static Name key_DefaultConfiguration ("DefaultConfiguration");
    if(member == key_DefaultConfiguration) {
      if(!ParseConfiguration(makefile, &makefile->default_configuration_, reader)) return false;
      continue;
    }

    reader.ReportWarning(String::Format("unknown member '%s'", member.ToCString()).ToCString());

    if(!reader.SkipValue()) return false;
  }

  return reader.ReadEndObject();
}

bool Makefile::ParseFiles (Makefile* makefile, JsonReader& reader) {
  if(!reader.ReadStartArray()) return false;

  while(!reader.IsEndArray()) {
    if(!reader.ReadStartObject()) return false;
    
    String path;
    String group;
    bool directory = false;
    bool excluded = false;
    bool glue = true;
    PrecompiledHeader precompiled_header = PrecompiledHeader::Use;

    while(!reader.IsEndObject()) {
      Name member;
      if(!reader.ReadMember(member)) return false; 
      
      // Path
      static Name key_Path ("Path");
      if(member == key_Path) {
        if(!reader.ReadValueString(path)) return false;
        path = Path::Combine(Path::GetDirectoryName(makefile->path_), path);
        path = Path::Canonical(path);
        continue;        
      }

      // Group
      static Name key_Group ("Group");
      if(member == key_Group) {
        if(!reader.ReadValueString(group)) return false;
        continue;
      }

      // Directory
      static Name key_Directory ("Directory");
      if(member == key_Directory) {
        if(!reader.ReadValueBoolean(directory)) return false;
        continue;
      }

      // Excluded
      static Name key_Excluded ("Excluded");
      if(member == key_Excluded ) {
        if(!reader.ReadValueBoolean(excluded)) return false;
        continue;
      }

      // Excluded
      static Name key_Glue ("Glue");
      if(member == key_Glue) {
        if(!reader.ReadValueBoolean(glue)) return false;
        continue;
      }

      // PrecompiledHeader
      static Name key_PrecompiledHeader("PrecompiledHeader");
      if(member == key_PrecompiledHeader) {
        String value;
        if(!reader.ReadValueString(value)) return false;
        if(value.Equals("Use",StringComparison::OrdinalIgnoreCase)) {
          precompiled_header = PrecompiledHeader::Use;
        } else if(value.Equals("Create",StringComparison::OrdinalIgnoreCase)) {
          precompiled_header = PrecompiledHeader::Create;
        } else if(value.Equals("None",StringComparison::OrdinalIgnoreCase)) {
          precompiled_header = PrecompiledHeader::None;
        } 
        continue;
      }

      reader.ReportWarning(String::Format("unknown member '%s'", member.ToCString()).ToCString());

      if(!reader.SkipValue()) return false;
    }

    if(!reader.ReadEndObject()) return false;    

    if(excluded) {
      makefile->RemoveBuildFile(Path::Canonical(Path::GetRelativePath(path,makefile->target_dir_)));
      continue;
    }
    
    // Entire directory
    if(Directory::Exists(path)) {
      if(directory) {
        BuildFile* file = makefile->GetBuildFile(Path::Canonical(Path::GetRelativePath(path,makefile->target_dir_)));
        file->group_ = group;
        file->precompiled_header_ = PrecompiledHeader::None;
        file->directory_ = directory;
        file->glue_ = glue;
        
      } else {
        std::vector<String> files = Directory::GetFiles(path);
        for(noz_uint32 i=0,c=files.size(); i<c; i++) {        
          BuildFile* file = makefile->GetBuildFile(Path::Canonical(Path::GetRelativePath(files[i],makefile->target_dir_)));
          file->group_ = group;
          file->precompiled_header_ = precompiled_header;
          file->directory_ = false;
          file->glue_ = glue;
        }
      }

    // Single file
    } else if (File::Exists(path)) {
      BuildFile* file = makefile->GetBuildFile(Path::Canonical(Path::GetRelativePath(path,makefile->target_dir_)));
      file->group_ = group;
      file->precompiled_header_ = precompiled_header;
      file->directory_ = false;
      file->glue_ = glue;
    }
  }

  return reader.ReadEndArray();
}

bool Makefile::ParseConfigurations (Makefile* makefile, JsonReader& reader) {
  if(!reader.ReadStartObject()) return false;

  while(!reader.IsEndObject()) {  
    Name configuration_name;
    if(!reader.ReadMember(configuration_name)) return false;

    Configuration* configuration = new Configuration;
    configuration->name_ = configuration_name;
  
    if(!ParseConfiguration(makefile, configuration, reader)) {
      delete configuration;
      return false;
    }
    makefile->configurations_.push_back(configuration);
  }

  return reader.ReadEndObject();
}

bool Makefile::ParseConfiguration (Makefile* makefile, Configuration* configuration, JsonReader& reader) {
  if(!reader.ReadStartObject()) return false;
    
  while(!reader.IsEndObject()) {
    Name member;
    if(!reader.ReadMember(member)) return false; 
      
    // PreprocessorDefinitions
    static Name key_PreprocessorDefinitions ("PreprocessorDefinitions");
    if(member == key_PreprocessorDefinitions) {
      if(!reader.ReadStartArray()) return false;
      while(!reader.IsEndArray()) {
        String value;
        if(!reader.ReadValueString(value)) return false;
        configuration->preprocessor_definitions_.push_back(value);
      }
      if(!reader.ReadEndArray()) return false;
      continue;        
    }

    // Optimizations
    static Name key_Optimizations ("Optimizations");
    if(member == key_Optimizations) {
      configuration->optimizations_is_default_ = false;
      if(!reader.ReadValueBoolean(configuration->optimizations_)) return false;
      continue;        
    }

    // Warnings
    static Name key_Warnings ("Warnings");
    if(member == key_Warnings) {
      configuration->warnings_is_default_ = false;
      if(!reader.ReadValueBoolean(configuration->warnings_)) return false;
      continue;        
    }

    // IncludeDirectories
    static Name key_IncludeDirectories ("IncludeDirectories");
    if(member == key_IncludeDirectories) {
      if(!reader.ReadStartArray()) return false;
      while(!reader.IsEndArray()) {
        String path;
        if(!reader.ReadValueString(path)) return false;
        path = Path::Canonical(Path::Combine(Path::GetDirectoryName(makefile->path_),path));
        path = Path::Canonical(Path::GetRelativePath(path,makefile->target_dir_));
        configuration->include_directories_.push_back(path);
      }
      if(!reader.ReadEndArray()) return false;
      continue;        
    }

    // Libraries
    static Name key_Libraries ("Libraries");
    if(member == key_Libraries) {
      if(!reader.ReadStartArray()) return false;
      while(!reader.IsEndArray()) {
        String lib;
        if(!reader.ReadValueString(lib)) return false;
        configuration->libraries_.push_back(lib);
      }
      if(!reader.ReadEndArray()) return false;
      continue;        
    }

    // PrecompiledHeader
    static Name key_PrecompiledHeader ("PrecompiledHeader");
    if(member == key_PrecompiledHeader) {
      configuration->precompiled_header_is_default_ = false;
      String value;
      if(!reader.ReadValueString(value)) return false;
      if(value.Equals("Use",StringComparison::OrdinalIgnoreCase)) {
        configuration->precompiled_header_ = PrecompiledHeader::Use;
      } else if(value.Equals("Create",StringComparison::OrdinalIgnoreCase)) {
        configuration->precompiled_header_ = PrecompiledHeader::Create;
      } else if(value.Equals("None",StringComparison::OrdinalIgnoreCase)) {
        configuration->precompiled_header_ = PrecompiledHeader::None;
      } 
      continue;        
    }

    // PrecompiledHeaderFile
    static Name key_PrecompiledHeaderFile ("PrecompiledHeaderFile");
    if(member == key_PrecompiledHeaderFile) {
      configuration->precompiled_header_file_is_default_ = false;
      if(!reader.ReadValueString(configuration->precompiled_header_file_)) return false;
      continue;        
    }

    reader.ReportWarning(String::Format("unknown member '%s'", member.ToCString()).ToCString());

    if(!reader.SkipValue()) return false;
  }

  return reader.ReadEndObject();
}

bool Makefile::MergeConfiguration (Configuration* src, Configuration* dst) {
  noz_assert(src);
  noz_assert(dst);

  if(!src->bin_directory_.IsEmpty() && dst->bin_directory_.IsEmpty()) dst->bin_directory_ = src->bin_directory_;
  if(!src->output_directory_.IsEmpty() && dst->output_directory_.IsEmpty()) dst->output_directory_ = src->output_directory_;

  if(!src->include_directories_.empty()) {
    dst->include_directories_.insert(dst->include_directories_.end(), src->include_directories_.begin(), src->include_directories_.end());
  }

  if(!src->libraries_.empty()) {
    dst->libraries_.insert(dst->libraries_.end(), src->libraries_.begin(), src->libraries_.end());
  }

  if(!src->preprocessor_definitions_.empty()) {
    dst->preprocessor_definitions_.insert(dst->preprocessor_definitions_.end(), src->preprocessor_definitions_.begin(), src->preprocessor_definitions_.end());
  }

  if(dst->warnings_is_default_) dst->warnings_ = src->warnings_;
  if(dst->optimizations_is_default_) dst->optimizations_ = src->optimizations_;
  if(dst->precompiled_header_file_is_default_) dst->precompiled_header_file_ = src->precompiled_header_file_;
  if(dst->precompiled_header_is_default_) dst->precompiled_header_ = src->precompiled_header_;

  return true;
}

String Makefile::Preprocess (const char* p, const ParseOptions& options) {
  struct PreprocessHelper {
    static bool IsWhitespace (const char* p) {return *p=='\n' || *p=='\r' || *p==' ' || *p == '\t';}
  };

  StringBuilder sb;
  StringBuilder sbkey;
  std::vector<bool> include;
  include.push_back(true);
  String target = PlatformTypeToString(options.platform);

  while(*p) {
    bool append = include.back();

    // Copy quoted strings directly
    if(*p == '\"') {
      if(append) sb.Append(*p);
      for(p++; *p && !(*p=='\"' && *(p-1)!='\\'); p++) if(append) sb.Append(*p);
      if(*p) {if(append) sb.Append(*p); p++;}
      continue;
    }

    // Strip out line comments
    if(*p == '#') {
      for(;*p && *p!='\n';p++);
      continue;
    }

    // Check for end of nested include
    if(include.size() > 1 && *p == ')') {
      p++;
      include.pop_back();
      continue;
    }

    // If its not a preprocessor macro then just add it
    if(*p != '@') {
      if(append) sb.Append(*p);
      p++;
      continue;
    }

    // Not-Defined?
    bool not_defined = false;
    if(*(++p) == '!') {
      not_defined = true;
      p++;
    }

    // Extract the key.
    sbkey.Clear();
    for(; *p && !PreprocessHelper::IsWhitespace(p) && *p != '(' && *p != '!'; p++) sbkey.Append(*p);
    String key = sbkey.ToString();

    // Skip whitespace
    for(; PreprocessHelper::IsWhitespace(p); p++);

    // Ensure macro is valid.
    if(*p != '(') continue;

    p++;

    if(not_defined) {
      include.push_back(!(options.defines.find(key) != options.defines.end() || target.Equals(key,StringComparison::OrdinalIgnoreCase)));
    } else {
      include.push_back(!(options.defines.find(key) == options.defines.end() && !target.Equals(key,StringComparison::OrdinalIgnoreCase)));
    }
  }

  return sb.ToString();
}

Makefile::BuildFile* Makefile::GetBuildFile (const String& path) {
  auto it = files_by_name_.find(path);
  if(it == files_by_name_.end()) {
    BuildFile* file = new BuildFile;
    file->path_ = path;
    files_.push_back(file);
    files_by_name_[path] = file;
    return file;
  }
  return it->second;
}

void Makefile::RemoveBuildFile(const String& path) {
  auto it = files_by_name_.find(path);
  if(it == files_by_name_.end()) return;
  files_by_name_.erase(it);

  for(noz_uint32 i=0,c=files_.size(); i<c; i++) {
    if(files_[i]->path_.Equals(path)) {
      delete files_[i];
      files_.erase(files_.begin()+i);
      break;
    }
  }
}

namespace noz {
  static String PlatformTypeUnknown ("Unknown");
  static String PlatformTypeWindows ("Windows");
  static String PlatformTypeOSX ("OSX");
  static String PlatformTypeIOS ("IOS");
}

const String& Makefile::PlatformTypeToString(PlatformType pt) {
  switch(pt) {
    default: break;
    case PlatformType::Windows: return PlatformTypeWindows;
    case PlatformType::OSX: return PlatformTypeOSX;
    case PlatformType::IOS: return PlatformTypeIOS;
  }
  return PlatformTypeUnknown;
}

Makefile::PlatformType Makefile::StringToPlatformType (const char* s) {
  if(PlatformTypeWindows.Equals(s,StringComparison::OrdinalIgnoreCase)) return PlatformType::Windows;
  if(PlatformTypeIOS.Equals(s,StringComparison::OrdinalIgnoreCase)) return PlatformType::IOS;
  if(PlatformTypeOSX.Equals(s,StringComparison::OrdinalIgnoreCase)) return PlatformType::OSX;
  return PlatformType::Unknown;
}

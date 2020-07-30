///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/Directory.h>
#include <noz/IO/StreamReader.h>
#include <noz/IO/MemoryStream.h>
#include <noz/Text/Json/JsonObject.h>
#include <noz/Text/Json/JsonArray.h>
#include <noz/Text/Json/JsonString.h>

#include "MakeProject.h"

using namespace noz;

#define REQUIRE_STRING(x,y) \
  if(nullptr==(x)->Get<JsonString>(y)) { \
    printf("mising " #y); \
    return false; \
  }

String ResolveAutoGroupName (const String& path, const String& auto_group_dir, const String& auto_group_base) {
  String result;
  if(!auto_group_dir.IsEmpty()) {
    result = Path::GetRelativePath(
      Path::Canonical(path),
      auto_group_dir);
    if(result[0]=='.') result = String::Empty;
  }
        
  if(!auto_group_base.IsEmpty()) {
    result = Path::Combine(auto_group_base, result);
  }

  return result;
}

bool ResolveFiles (JsonObject* makefile) {
  JsonObject* cfgs = makefile->Get<JsonObject>("CONFIGURATIONS");

  // Get files array.
  JsonArray* files = makefile->Get<JsonArray>("FILES");
  noz_assert(files);

  String source_dir = makefile->GetString("MAKEFILE_DIR");
  String target_dir = makefile->GetString("PROJECT_DIR");

  String auto_group_dir = makefile->GetString("AUTO_GROUP_DIR");
  String auto_group_base = makefile->GetString("AUTO_GROUP_BASE");

  JsonObject* resolved_files = new JsonObject;
  for(auto itf=files->GetValues().begin(); itf!=files->GetValues().end(); itf++) {
    if(!(*itf)->IsObject()) {
      Console::WriteLine("FILES: elements must be of JsonObject type");
      return false;
    }

    JsonObject* file = (JsonObject*)(*itf);
    JsonString* file_path = file->Get<JsonString>("PATH");
    JsonObject* file_cfgs = file->Get<JsonObject>("CONFIGURATIONS");

    String group_name = file->GetString("GROUP");

    bool exclude_all = false;

    // Default file configuration..
    JsonObject* default_cfg = file->Get<JsonObject>("DEFAULT_CONFIGURATION");
    if(default_cfg) {
      // To simplify the writing process if there is a default configuration on a file 
      // then the file will alwasy have a CONFIGURATIONS object on it as well.
      if(nullptr == file_cfgs) {
        file_cfgs = new JsonObject;
        file->Add("CONFIGURATIONS", file_cfgs);
      }

      exclude_all = default_cfg->GetBoolean("EXCLUDED");

      // To simplify file writing if there is a DEFAULT_CONFIGURATION then an entry 
      // in the PROJECT CONFIGURATIONS object will be created using the default configuration.  If
      // the entry already exists the the default configuration will be cloned into it.
      for(auto it=cfgs->GetValues().begin(); it!=cfgs->GetValues().end(); it++) {
        JsonObject* file_cfg = file_cfgs->Get<JsonObject>(it->first);
        if(nullptr == file_cfg) {
          file_cfg = (JsonObject*)default_cfg->Clone();
          file_cfgs->Add(it->first, file_cfg);
        } else {
          default_cfg->CloneInto(file_cfg,false);
        }

        exclude_all &= file_cfg->GetBoolean("EXCLUDED");
      }
    }

    String full_path = Path::Canonical(Path::Combine(source_dir,file_path->Get()));
    String ext = Path::GetExtension(full_path);

    // Some directories act like files..
    bool directory_as_file = ext.Equals(".xcassets", StringComparison::OrdinalIgnoreCase);

    // If the path is a directory..
    if(!directory_as_file && Directory::Exists(full_path)) {
      std::vector<String> files = Directory::GetFiles(full_path);
      
      if(group_name.IsEmpty()) {        
        group_name = ResolveAutoGroupName(file_path->Get(), auto_group_dir, auto_group_base);
      }

      for(auto it=files.begin(); it!=files.end(); it++) {
        String rel_path = Path::Canonical(Path::GetRelativePath((*it),target_dir));

        // If this file is excluded from all configurations then dont add it to the project..
        if(exclude_all) {
          resolved_files->Remove(rel_path);
          continue;
        }

        // Add a new file object
        JsonObject* fo = resolved_files->Get<JsonObject>(rel_path);
        if(nullptr == fo) {
          if(exclude_all) {
            continue;
          }

          fo = new JsonObject;
          resolved_files->Add(rel_path,fo);
        }

        fo->Add ("PATH", rel_path);
        fo->Add ("FULL_PATH", (*it));
        fo->Add ("EXT", Path::GetExtension(rel_path));

        if(file_cfgs) fo->Add("CONFIGURATIONS", file_cfgs->Clone());
        if(!group_name.IsEmpty()) fo->Add ( "GROUP", group_name);
      }
    } else {
      String rel_path = Path::Canonical(Path::GetRelativePath(full_path,target_dir));

      // If the file is excluded from all projects then remove it from the resolved files list.
      if(exclude_all) {
        resolved_files->Remove(rel_path);
        continue;
      }

      // Add a new file object
      JsonObject* fo = resolved_files->Get<JsonObject>(rel_path);
      if(nullptr == fo) {
        fo = new JsonObject;
        resolved_files->Add(full_path,fo);
      }
      fo->Add ("PATH", rel_path);
      fo->Add ("FULL_PATH", full_path);
      fo->Add ("EXT", Path::GetExtension(rel_path));

      if(file_cfgs) fo->Add("CONFIGURATIONS", file_cfgs->Clone());

      if(group_name.IsEmpty()) {        
        group_name = ResolveAutoGroupName(Path::GetDirectoryName(file_path->Get()), auto_group_dir, auto_group_base);
      }

      if(!group_name.IsEmpty()) fo->Add ( "GROUP", group_name);
    }
  }
  
  // Replace old files with resolved files
  makefile->Set("FILES",resolved_files);  

  return true;
}

String ResolvePath (JsonObject* makefile, const String& path) { 
  return Path::Canonical(
    Path::GetRelativePath(
      Path::Canonical(
        Path::Combine(
          makefile->GetString("MAKEFILE_DIR"), 
          path
        )
      ),
      makefile->GetString("PROJECT_DIR")
    )
  );
}

bool Process (JsonObject* makefile) {
  noz_assert(makefile);

  if(!makefile->Contains("PROJECT_DIR")) {
    makefile->SetString("PROJECT_DIR", makefile->GetString("TARGET_PLATFORM"));
  }    

  // Resolve the project directory to a full path.
  makefile->SetString("PROJECT_DIR", Path::Canonical(Path::Combine(makefile->GetString("MAKEFILE_DIR"), makefile->GetString("PROJECT_DIR"))));

  // TARGET_TYPE
  String target_type = makefile->GetString("TARGET_TYPE");
  if(!target_type.Equals("LIB",StringComparison::OrdinalIgnoreCase) && 
     !target_type.Equals("APPLICATION",StringComparison::OrdinalIgnoreCase) &&
     !target_type.Equals("CONSOLE",StringComparison::OrdinalIgnoreCase) ) {
    Console::WriteLine("TARGET_TYPE: unknown type '%s'", target_type.ToCString() );
    return false;
  }

  // TARGET_NAME
  if(nullptr == makefile->Get<JsonString>("TARGET_NAME")) {
    // Copy the make file name into the project name if there is no name
    makefile->SetString("TARGET_NAME", makefile->GetString("MAKEFILE_NAME"));
  }

  // Project must have configurations..
  JsonObject* cfgs = makefile->Get<JsonObject>("CONFIGURATIONS");
  if(nullptr == cfgs) {
    return false;
  }

  // Validate all configurations are objects and have names
  for(auto it=cfgs->GetValues().begin(); it!=cfgs->GetValues().end(); it++) {
    if(!it->second->IsObject()) {
      return false;
    }
  }    

  // If there is a default configuration expand it into all of the specified configurations.
  JsonObject* default_cfg = makefile->Get<JsonObject>("DEFAULT_CONFIGURATION");
  if(default_cfg) {
    for(auto it=cfgs->GetValues().begin(); it!=cfgs->GetValues().end(); it++) {
      default_cfg->CloneInto((JsonObject*)it->second,false);
    }    

    // Now that the default configuration has been merged into the configurations we 
    // no longer need it.
    makefile->Remove("DEFAULT_CONFIGURATION");
  }

  // Detailed validation of all configurations
  for(auto it=cfgs->GetValues().begin(); it!=cfgs->GetValues().end(); it++) {
    JsonObject* cfg = (JsonObject*)it->second;
    noz_assert(cfg);

    // Resolve the intermediates directory (alwasy relative to the project directory)
    String int_dir = cfg->GetString("INT_DIR");
    if(int_dir.IsEmpty()) int_dir = "output";
    int_dir = Path::Combine(int_dir,it->first);
    int_dir = Path::Combine(int_dir,makefile->GetString("TARGET_NAME"));
    int_dir = Path::Canonical(int_dir);
    cfg->SetString("INT_DIR", int_dir);

    // Generate the out dir
    String out_dir = cfg->GetString("OUT_DIR");
    if(out_dir.IsEmpty()) out_dir = "bin";
    out_dir = Path::Combine(out_dir,it->first);
    cfg->SetString("OUT_DIR",out_dir);

    // Copy name into object for convienence
    cfg->Add("NAME", it->first);

    // Add automatic preprocessor definitions
    JsonArray* ppd = cfg->Get<JsonArray>("PREPROCESSOR_DEFINITIONS");
    if(nullptr == ppd) {
      ppd = new JsonArray;
      cfg->Set("PREPROCESSOR_DEFINITIONS", ppd);
    }    
    ppd->Add(new JsonString(String::Format("NOZ_%s",makefile->GetString("TARGET_PLATFORM").ToCString()).Upper()));
    
    // Resolve include directories
    JsonArray* incs = cfg->Get<JsonArray>("INCLUDE_DIRECTORIES");
    JsonArray* resolved_incs = new JsonArray;
    if(incs) {
      for(auto it=incs->GetValues().begin(); it!=incs->GetValues().end(); it++) {
        if(!(*it)->IsString()) return false;

        JsonString* inc = new JsonString;
        inc->Set(ResolvePath(makefile,((JsonString*)*it)->Get()));
        resolved_incs->Add(inc);
      }

      cfg->Add("INCLUDE_DIRECTORIES", resolved_incs);
    }

    // If generating glue add the glue output directory as a include directory
    if(makefile->GetBoolean("GLUE")) {
      resolved_incs->Add(
        new JsonString(Path::Combine(int_dir,"glue"))
      );
    }
  }

  // Process files...
  JsonArray* files = makefile->Get<JsonArray>("FILES");
  if(nullptr == files) {
    return false;
  }

  // References...
  if(makefile->Contains("REFERENCES")) {
    JsonValue* jv = makefile->Get("REFERENCES");
    if(!jv->IsArray()) {
      return false;
    }
    JsonArray* refs = (JsonArray*)jv;
    for(auto it=refs->GetValues().begin(); it!=refs->GetValues().end(); it++) {
      if(!(*it)->IsObject()) return false;

      JsonObject* ref = (JsonObject*)(*it);
      JsonString* path = ref->Get<JsonString>("PATH");
      if(nullptr==path) {
        return false;
      }

      // Resolve the path to be a full path.
      ref->Add("PATH", Path::Canonical(Path::Combine(makefile->GetString("MAKEFILE_DIR"), path->Get())));
    }
  }

  return true;
}


bool Preprocess(char* p, const Name& platform, const std::set<Name>& defines) {
  struct PreprocessHelper {
    static bool IsWhitespace (const char* p) {
      return *p=='\n' || *p=='\r' || *p==' ' || *p == '\t';
    }
  };

  int nested_macro = true;
  char* pp = p;
  while(nested_macro) {
    nested_macro = false;
    p = pp;

    while(*p) {
      // Skip quoted strings..
      if(*p=='\"') {
        for(p++;p && !(*p=='\"' && *(p-1)!='\\');p++);
        if(*p) p++;
        continue;
      }

      // Null out line comments
      if(*p == '#') {
        for(;*p && *p!='\n';p++) *p=' ';
        continue;
      }

      if(*p != '@') {
        p++;
        continue;
      }

      char* ms = p;
      char* s = p;

      // Skip until whitespace is found or the open paren
      for(p++;*p && !PreprocessHelper::IsWhitespace(p) && *p != '(' && *p != '!';p++);

      // Look for "NOT" defined
      bool not_defined = false;
      char* ks = s;
      if(*p == '!') {
        not_defined = true;
        for(p++;*p && !PreprocessHelper::IsWhitespace(p) && *p != '(';p++);
        ks++;
      }

      // Extract the key
      String key(ks+1,0,p-ks-1);

      // Skip whitespace until the opening paren
      for(;*p && *p != '(';p++) {
        if(!PreprocessHelper::IsWhitespace(p)) return false;
      }

      bool erase = false;
      
      // Handle not-defined
      if(not_defined) {
        erase = defines.find(key) != defines.end() || platform.ToString().Equals(key,StringComparison::OrdinalIgnoreCase);
      } else {
        erase = defines.find(key) == defines.end() && !platform.ToString().Equals(key,StringComparison::OrdinalIgnoreCase);
      }

      // Clear all values from '@' to '('    
      for(p++;s<p;s++) *s = ' ';

      // Find closing paren
      for(int depth=1; *p; p++) {
        if(*p=='(') depth++;
        else if(*p==')') {depth--;if(depth==0) break;}
        else if(*p=='@') nested_macro=nested_macro | !erase;
      }

      // Erase ')'
      *(p++) = ' ';

      // Erase entire macro?
      if(erase) for(;s<p;s++) *s = ' ';
    }
  } 

  return true;
}


JsonObject* MakeProject::Load (const String& path, const Name& platform) {
  std::set<Name> defines;
  return Load(path,platform,defines);
}

JsonObject* MakeProject::Load (const String& path, const Name& platform, const std::set<Name>& defines) {
  String full_path = Path::Canonical(Path::Combine(Environment::GetCurrentDirectory(),path));

  FileStream fs;
  if(!fs.Open(full_path,FileMode::Open)) return nullptr;

  noz_uint32 size = fs.GetLength();
  if(size == 0) {
    Console::WriteLine("%s: error: empty make file", full_path.ToCString() );
    return nullptr;
  }

  // Load the entire input stream into a memory stream.
  MemoryStream ms(size+1);
  ms.SetLength(size);
  fs.Read((char*)ms.GetBuffer(), 0, size);
  ((char*)ms.GetBuffer())[size] = 0;
  fs.Close();

  if(!Preprocess((char*)ms.GetBuffer(),platform,defines)) {
    Console::WriteLine("%s: error: preprocessing failed", full_path.ToCString() );
    return nullptr;
  }

  StreamReader reader(&ms);
  JsonValue* jv = JsonValue::Load(reader);
  if(!jv || !jv->IsObject()) {
    Console::WriteLine("%s: error: Json parser error", full_path.ToCString() );
    delete jv;
    return nullptr;
  }

  // Save makefile properties..
  JsonObject* makefile = (JsonObject*)jv;
  makefile->SetString("MAKEFILE_PATH", full_path);
  makefile->SetString("MAKEFILE_DIR", Path::GetDirectoryName(full_path));
  makefile->SetString("MAKEFILE_NAME", Path::GetFilenameWithoutExtension(full_path));

  // Defines..
  JsonObject* makefile_defines = new JsonObject;
  for(auto it=defines.begin(); it!=defines.end(); it++) {
    makefile_defines->Add(*it,"");
  }
  makefile->Add("MAKEFILE_DEFINES", makefile_defines);

  // TARGET_PLATFORM
  makefile->SetString("TARGET_PLATFORM", platform.ToString());

  if(!Process(makefile)) {
    delete jv;
    return nullptr;
  }

  ResolveFiles(makefile);

  return makefile;
}


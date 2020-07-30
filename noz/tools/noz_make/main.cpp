///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Editor/Tool/ProjectGen/ProjectGen.h>
#include <noz/Editor/Tool/GlueGen/GlueGen.h>
#include <noz/IO/Directory.h>

using namespace noz;
using namespace noz::Editor;

typedef std::multimap<Name,String> OptionsMap;

void ParseOptions (int argc, const char* argv[], OptionsMap& out) {
  for(noz_int32 i=2; i<argc; i++) {
    if(argv[i][0] != '-') {
      out.insert(OptionsMap::value_type(Name::Empty,argv[i]));
      continue;
    } 

    // Separate option and value for the format -option=<value>
    String option(argv[i]+1);
    String value;
    noz_int32 equal;
    if(-1 !=(equal=option.IndexOf('='))) {
      value = option.Substring(equal+1);
      option = option.Substring(0,equal);
    }

    out.insert(OptionsMap::value_type(option,value));
  }
}

bool ProjectsMain (OptionsMap& options_map) {
  Makefile::ParseOptions options;
  String makefile_path;

  for(auto it=options_map.begin(); it!=options_map.end(); it++) {
    if(it->first == Name::Empty) {
      makefile_path = Path::Canonical(Path::Combine(Environment::GetCurrentDirectory(), it->second));
      continue;
    }

    // -define=<define>
    const String& option = it->first.ToString();
    const String& value = it->second;
    
    if(option.Equals("define",StringComparison::OrdinalIgnoreCase)) {
      options.defines.insert(it->second);
      continue;

    // -target=<target>
    } else if (option.Equals("platform",StringComparison::OrdinalIgnoreCase)) {
      options.platform = Makefile::StringToPlatformType(value.ToCString());
    }
  }

  if(makefile_path.IsEmpty()) {
    Console::WriteLine("error: no input file");
    return false;
  }

  return ProjectGen::Generate(makefile_path, options);
}

int GlueMain(OptionsMap& options_map) {
  GlueGen::Options options;
  options.clean_ = false;
  options.platform_ = Makefile::PlatformType::Windows;
  String project_path;

  for(auto it=options_map.begin(); it!=options_map.end(); it++) {
    if(it->first == Name::Empty) {
      project_path = Path::Canonical(Path::Combine(Environment::GetCurrentDirectory(), it->second));
      continue;
    }

    // -define=<define>
    const String& option = it->first.ToString();
    const String& value = it->second;
    
    if(option.Equals("define",StringComparison::OrdinalIgnoreCase)) {
      options.defines_.insert(it->second);
      continue;

    // -platform=<platform>
    } else if (option.Equals("platform",StringComparison::OrdinalIgnoreCase)) {
      options.platform_ = Makefile::StringToPlatformType(value.ToCString());
      continue;

    // -config=<config>
    } else if (option.Equals("config",StringComparison::OrdinalIgnoreCase)) {
      options.config_ = value;
      continue;

    // -clean
    } else if (option.Equals("clean",StringComparison::OrdinalIgnoreCase)) {
      options.clean_ = true;
      continue;
    }
  }

  return !GlueGen::Generate(project_path, options);
}

int main(int argc, const char* argv[]) {
  // Must specify a subcommand
  if(argc<2) {
    Console::WriteLine("Type 'noz_make help' for usage.");
    return 0;
  }

  Environment::Initialize(argc, argv);
 
  OptionsMap options;
  ParseOptions(argc,argv,options);

  String subcmd(argv[1]);
  if(subcmd.Equals("help",StringComparison::OrdinalIgnoreCase)) {
  } else if(subcmd.Equals("glue",StringComparison::OrdinalIgnoreCase)) {
    return GlueMain(options);
  } else if(subcmd.Equals("projects",StringComparison::OrdinalIgnoreCase)) {
    return !ProjectsMain(options);
  } else {
    Console::WriteLine("Unknown subcommand: '%s'", argv[1]);
    Console::WriteLine("Type 'noz_make help' for usage.");
  }

  return 0;
}


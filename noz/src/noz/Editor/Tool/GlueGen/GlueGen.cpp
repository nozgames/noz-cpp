///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "GlueGen.h"

using namespace noz;
using namespace noz::Editor;


void GlueGen::ReportError (GlueFile* gf, noz_int32 line, const char* format, ...) {
  char msg[2048];
  va_list args;	
	va_start(args,format);
#if defined(NOZ_WINDOWS)
  vsprintf_s(msg,2048,format,args);
#else
  vsprintf(msg,format,args);
#endif
  va_end(args);

  Console::WriteLine("%s(%d): error: %s", gf->GetFullPath().ToCString(), line, msg);    
}

bool GlueGen::SetOptions (GlueState* gs, const String& makefile_path, const Options& options) {
  gs->platform_ = options.platform_;
  gs->defines_ = options.defines_;
  gs->config_name_ = options.config_;
  gs->makefile_path_ = makefile_path;
  gs->makefile_directory_ = Path::GetDirectoryName(makefile_path);
  gs->clean_ = options.clean_;
  gs->output_path_ = options.output_path_;

  // Ensure an input file was given.
  if(gs->makefile_path_.IsEmpty()) {
    Console::WriteLine("error: missing input file");
    return false;
  }

  // Ensure a config was given
  if(gs->config_name_.IsEmpty()) {
    Console::WriteLine("error: no configuration specified (use -config=<name> to specify a configuration)");
    return false;
  }

  // Ensure the input file exists and is a visual studio project file
  if(Path::GetExtension(gs->makefile_path_).CompareTo(".nozmake")) {
    Console::WriteLine("%s: error: file is not a NoZ Makefile (*.nozmake)", gs->makefile_path_.ToCString());
    return false;
  }

  // Ensure the file exists.
  if(!File::Exists(gs->makefile_path_)) {
    Console::WriteLine("%s: error: file not found", gs->makefile_path_.ToCString() );
    return false;
  }

  return true;
}

bool GlueGen::Generate(const String& makefile_path, const Options& options) {
  // Initialize the reserved names in the meta
  GlueMeta::InitializeReservedNames();

  // Initialzie the glue state
  GlueState gs;

  // Parse the command line options
  if(!SetOptions(&gs,makefile_path,options)) return false;

  // Parse the input project
  if(!gs.LoadProject()) return false;

  // Check if a build is required
  if(!gs.IsBuildRequired()) return true;

  // Search all include and compile files for include files.
  if(!ParseIncludedFiles (&gs)) return false;    

  // Parse all declarations to ensure all classes and enums are known before
  // we begin building structures..
  if(!ParseDeclarations(&gs)) return false;

  // Perform full parse now.
  if(!Parse(&gs)) return false;

  // Process all classes.
  if(!gs.ProcessClasses()) return false;

  // Write to the target
  if(!Write(&gs)) return false;

  return true;
}

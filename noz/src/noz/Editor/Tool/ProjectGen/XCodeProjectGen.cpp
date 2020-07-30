///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Guid.h>
#include <noz/Editor/Tool/Makefile.h>
#include <noz/IO/StreamWriter.h>
#include "XCodeProjectGen.h"

using namespace noz;

String XCodeProjectGen::BuildPhaseToString (BuildPhase p) const {
  switch(p) {
    case BuildPhase::CopyFiles: { static const String s("CopyFiles"); return s;}
    case BuildPhase::Resources: { static const String s("Resources"); return s;}
    case BuildPhase::Sources: { static const String s("Sources"); return s;}
    case BuildPhase::Unknown: { static const String s("Unknown"); return s;}
    case BuildPhase::Frameworks: { static const String s("Frameworks"); return s;}
  }
  return String::Empty;
}

void XCodeProjectGen::WritePBXValue (TextWriter& writer, const String& value) {
  bool quoted = value.IsEmpty() || -1 != value.IndexOf(' ') || -1 != value.IndexOf('$') || -1 != value.IndexOf('<') || -1 != value.IndexOf('>')  || -1 != value.IndexOf('-') || -1 != value.IndexOf(',');
  if(value[0] == '\"') quoted = false;

  if(quoted) writer.Write("\"");
  writer.Write(value);
  if(quoted) writer.Write("\"");
}

void XCodeProjectGen::WritePBXProperty (TextWriter& writer, noz_int32 indent, const String& name, const String& value, const String& comment) {
  writer.Write('\t', indent);
  writer.Write(name);
  writer.Write(" = ");
  WritePBXValue(writer,value);
  if(!comment.IsEmpty()) {
    writer.Write(" /* ");
    writer.Write(comment);
    writer.Write(" */");
  }
  writer.Write(";\n");
}

void XCodeProjectGen::WritePBXProperty (TextWriter& writer, const String& name, const String& value, const String& comment) {
  writer.Write(name);
  writer.Write(" = ");
  WritePBXValue(writer,value);
  if(!comment.IsEmpty()) {
    writer.Write(" /* ");
    writer.Write(comment);
    writer.Write(" */");
  }
  writer.Write("; ");
}

void XCodeProjectGen::WritePBXListPropertyPrologue(TextWriter& writer, noz_int32 indent, const String& name) {
  writer.Write('\t',indent);
  writer.Write(name);
  writer.Write(" = (\n");
}

void XCodeProjectGen::WritePBXListProperty(TextWriter& writer, noz_int32 indent, const String& value, const String& comment) {
  writer.Write('\t',indent);
  WritePBXValue(writer,value);
  if(!comment.IsEmpty()) {
    writer.Write (" /* ");
    writer.Write(comment);
    writer.Write(" */");
  }
  writer.Write(",\n");
}

void XCodeProjectGen::WritePBXListPropertyEpilogue(TextWriter& writer, noz_int32 indent) {
  writer.Write('\t',indent);
  writer.Write(");\n");
}

void XCodeProjectGen::WritePBXElementPrologue (TextWriter& writer, noz_int32 indent, bool single_line, const String& guid, const String& comment) {
  writer.Write('\t',indent);
  if(guid.IsEmpty()) {
    writer.Write("{");
  } else {
    writer.Write(guid);
    if(!comment.IsEmpty()) {
      writer.Write(" /* ");
      writer.Write(comment);
      writer.Write(" */");
    }
    writer.Write(" = {");
  }
  if(!single_line) {
    writer.Write("\n");
  }
}

void XCodeProjectGen::WritePBXElementEpilogue (TextWriter& writer, noz_int32 indent) {
  writer.Write('\t', indent);
  writer.Write("};\n");
}

void XCodeProjectGen::WritePBXElementEpilogue (TextWriter& writer) {
  writer.Write("};\n");
}

bool XCodeProjectGen::WritePBXBuildFileSection (Project& project, TextWriter& writer) {
  writer.WriteLine("/* Begin PBXBuildFile section */");

  for(noz_uint32 i=0,c=project.build_files_.size(); i<c; i++) {
    const BuildFile& build_file = *project.build_files_[i];
    noz_assert(!build_file.guid_.IsEmpty());
    noz_assert(!build_file.file_reference_guid_.IsEmpty());
    noz_assert(!build_file.name_.IsEmpty());

    WritePBXElementPrologue(writer,2,true,build_file.guid_,String::Format("%s in %s", build_file.name_.ToCString(), BuildPhaseToString(build_file.phase_).ToCString()));
    WritePBXProperty(writer,"isa","PBXBuildFile");        
    WritePBXProperty(writer,"fileRef",build_file.file_reference_guid_,build_file.name_);
    WritePBXElementEpilogue(writer);
  }

  writer.WriteLine("/* End PBXBuildFile section */");

  return true;
}

bool XCodeProjectGen::WritePBXContainerItemProxySection (Project& project, TextWriter& writer) {
  if(project.makefile_->references_.empty()) return true;

  writer.Write("\n/* Begin PBXContainerItemProxy section */\n");

  for(noz_uint32 i=0,c=project.references_.size(); i<c; i++) {
    const Reference& ref = *project.references_[i];
    noz_assert(ref.makefile_);

    WritePBXElementPrologue(writer,2,false,ref.pbx_container_item_proxy_2_guid_,"PBXContainerItemProxy");
    WritePBXProperty(writer,3,"isa","PBXContainerItemProxy");
    WritePBXProperty(writer,3,"containerPortal", ref.container_portal_guid_, ref.build_file_->name_);
    WritePBXProperty(writer,3,"proxyType","2");
    WritePBXProperty(writer,3,"remoteGlobalIDString",ref.makefile_->xcode_product_guid_);
    WritePBXProperty(writer,3,"remoteInfo",ref.makefile_->name_);
    WritePBXElementEpilogue(writer,2);

    WritePBXElementPrologue(writer,2,false,ref.pbx_container_item_proxy_1_guid_,"PBXContainerItemProxy");
    WritePBXProperty(writer,3,"isa","PBXContainerItemProxy");
    WritePBXProperty(writer,3,"containerPortal", ref.container_portal_guid_, String::Format("%s.xcodeproj",ref.makefile_->name_.ToCString()));
    WritePBXProperty(writer,3,"proxyType","1");
    WritePBXProperty(writer,3,"remoteGlobalIDString",ref.makefile_->xcode_native_target_guid_);
    WritePBXProperty(writer,3,"remoteInfo",ref.makefile_->name_);
    WritePBXElementEpilogue(writer,2);
  }

  writer.Write("/* End PBXContainerItemProxy section */\n");

  return true;
}

bool XCodeProjectGen::WritePBXCopyFilesBuildPhaseSection (Project& project, TextWriter& writer) {
  if(project.copy_file_build_phases_.empty()) return true;

  writer.Write("\n/* Begin PBXCopyFilesBuildPhase section */\n");

  for(noz_uint32 i=0,c=project.copy_file_build_phases_.size(); i<c; i++) {
    const CopyFilesBuildPhase& phase = *project.copy_file_build_phases_[i];

    WritePBXElementPrologue(writer,2,false,phase.guid_,"CopyFiles");
    WritePBXProperty(writer,3,"isa","PBXCopyFilesBuildPhase");
    WritePBXProperty(writer,3,"buildActionMask","2147483647");
    WritePBXProperty(writer,3,"dstPath",phase.dst_path_);
    WritePBXProperty(writer,3,"dstSubfolderSpec","7");
    WritePBXListPropertyPrologue(writer,3,"files");
    WritePBXListProperty(writer,4,phase.file_guid_,"");
    WritePBXListPropertyEpilogue(writer,3);
    WritePBXProperty(writer,3,"runOnlyForDeploymentPostprocessing","0");
    WritePBXElementEpilogue(writer,2);
  }

  writer.Write("/* End PBXCopyFilesBuildPhase section */\n");
      
  return true;
}

bool XCodeProjectGen::WritePBXFileReferenceSection (Project& project, TextWriter& writer) {
  writer.WriteLine("\n/* Begin PBXFileReference section */");

  for(noz_uint32 i=0,c=project.file_references_.size(); i<c; i++) {
    const FileReference& file_ref = *project.file_references_[i];
    noz_assert(!file_ref.guid_.IsEmpty());

    WritePBXElementPrologue(writer,2,true,file_ref.guid_,file_ref.name_.IsEmpty() ? file_ref.path_ : file_ref.name_ );
    WritePBXProperty(writer, "isa", "PBXFileReference");

    if(file_ref.file_encoding_ >=0) WritePBXProperty(writer,"fileEncoding",String::Format("%d",file_ref.file_encoding_));
    if(!file_ref.explicit_file_type_.IsEmpty()) WritePBXProperty(writer,"explicitFileType", file_ref.explicit_file_type_);
    if(!file_ref.last_known_file_type_.IsEmpty()) WritePBXProperty(writer,"lastKnownFileType", file_ref.last_known_file_type_);
    if(file_ref.include_in_index_ >=0) WritePBXProperty(writer,"includeInIndex", String::Format("%d",file_ref.include_in_index_));
    if(!file_ref.name_.IsEmpty()) WritePBXProperty(writer,"name", file_ref.name_);
    if(!file_ref.path_.IsEmpty()) WritePBXProperty(writer,"path", file_ref.path_);
    if(!file_ref.source_tree_.IsEmpty()) WritePBXProperty(writer,"sourceTree", file_ref.source_tree_);

    WritePBXElementEpilogue(writer);
  }

  writer.WriteLine("/* End PBXFileReference section */");

  return true;
}

bool XCodeProjectGen::WritePBXFrameworksBuildPhaseSection (Project& project, TextWriter& writer) {
  writer.Write("\n/* Begin PBXFrameworksBuildPhase section */\n");

  WritePBXElementPrologue(writer,2,false,project.pbx_frameworks_build_phase_guid_,"Frameworks");
  WritePBXProperty(writer,3,"isa", "PBXFrameworksBuildPhase");
  WritePBXProperty(writer,3,"buildActionMask","2147483647");
  WritePBXListPropertyPrologue(writer,3,"files");

  for(noz_uint32 i=0,c=project.build_files_.size(); i<c; i++) {
    const BuildFile& build_file = *project.build_files_[i];
    if(build_file.phase_ != BuildPhase::Frameworks) continue;
    WritePBXListProperty(writer,4,build_file.guid_,String::Format("%s in Frameworks", build_file.name_.ToCString()));
  }

  WritePBXListPropertyEpilogue(writer,3);
  WritePBXProperty(writer,3,"runOnlyForDeploymentPostprocessing","0");
  WritePBXElementEpilogue(writer,2);

  writer.Write("/* End PBXFrameworksBuildPhase section */\n");
      
  return true;
}

bool XCodeProjectGen::WritePBXGroupSection (Project& project, TextWriter& writer) {
  // Write groups
  writer.Write("\n/* Begin PBXGroup section */\n");

  for(noz_uint32 i=0,c=project.groups_.size(); i<c; i++) {
    const Group& group = *project.groups_[i];
        
    WritePBXElementPrologue(writer,2,false,group.guid_,group.name_);
    WritePBXProperty(writer,3,"isa","PBXGroup");
    WritePBXListPropertyPrologue(writer,3,"children");

    // Child groups..
    for(noz_uint32 ii=0,cc=group.groups_.size(); ii<cc; ii++) {
      const Group& child = *group.groups_[ii];
      WritePBXListProperty(writer,4,child.guid_, child.name_);
    }

    // Child files.
    for(noz_uint32 ii=0,cc=group.file_references_.size(); ii<cc; ii++) {
      const FileReference& file_ref = *group.file_references_[ii];
      WritePBXListProperty(writer,4,file_ref.guid_, file_ref.name_.IsEmpty() ? file_ref.path_ : file_ref.name_);
    }

    WritePBXListPropertyEpilogue(writer,3);
        
    WritePBXProperty(writer,3,"name",group.name_);
    WritePBXProperty(writer,3,"sourceTree", "<group>");
    WritePBXElementEpilogue(writer,2);
  }

  writer.Write("/* End PBXGroup section */\n");

  return true;
}


bool XCodeProjectGen::WritePBXNativeTargetSection (Project& project, TextWriter& writer) {
  writer.Write("\n/* Begin PBXNativeTarget section */\n");

  WritePBXElementPrologue(writer,2,false,project.pbx_native_target_guid_, project.makefile_->name_);
  WritePBXProperty(writer,3,"isa","PBXNativeTarget");
  WritePBXProperty(writer,3,"buildConfigurationList", project.pbx_native_target_xconfiguration_list_guid_, String::Format("Build configuration list for PBXNativeTarget \"%s\"",project.makefile_->name_.ToCString()));
  WritePBXListPropertyPrologue(writer,3,"buildPhases");

  if(project.makefile_->glue_) {
    WritePBXListProperty(writer,4,project.glue_pbx_shell_script_build_phase_guid_, "ShellScript");
  }        

  WritePBXListProperty(writer,4,project.pbx_sources_build_phase_guid_, "Sources");
  WritePBXListProperty(writer,4,project.pbx_frameworks_build_phase_guid_, "Frameworks");
  if(project.has_resources_) {
    WritePBXListProperty(writer,4,project.pbx_resources_build_phase_guid_, "Resources");
  }
  for(auto it=project.copy_file_build_phases_.begin(); it!=project.copy_file_build_phases_.end(); it++) {
    WritePBXListProperty(writer,4,(*it)->guid_, "CopyFiles");
  }

  WritePBXListPropertyEpilogue(writer,3);

  WritePBXListPropertyPrologue(writer,3,"buildRules");
  WritePBXListPropertyEpilogue(writer,3);

  WritePBXListPropertyPrologue(writer,3,"dependencies");

  for(noz_uint32 i=0,c=project.references_.size(); i<c; i++) {
    const Reference& ref = *project.references_[i];
    WritePBXListProperty(writer, 4, ref.pbx_target_dependency_guid_, "PBXTargetDependency");
  }

  WritePBXListPropertyEpilogue(writer,3);
  WritePBXProperty(writer,3,"name", project.makefile_->name_);
  WritePBXProperty(writer,3,"productName", project.makefile_->name_);
  WritePBXProperty(writer,3,"productReference", project.product_pbx_file_reference_guid_, String::Format("%s.xcodeproj", project.makefile_->name_.ToCString()));
      
  if(project.makefile_->target_type_ == Makefile::TargetType::Application) {
    WritePBXProperty(writer,3,"productType","com.apple.product-type.application");
  } else if(project.makefile_->target_type_ == Makefile::TargetType::Console) {
    WritePBXProperty(writer,3,"productType","com.apple.product-type.tool");
  } else if(project.makefile_->target_type_ == Makefile::TargetType::Library) {
    WritePBXProperty(writer,3,"productType","com.apple.product-type.library.static");
  }
  WritePBXElementEpilogue(writer,2);

  writer.Write("/* End PBXNativeTarget section */\n");

  return true;
}

bool XCodeProjectGen::WritePBXProjectSection (Project& project, TextWriter& writer) {
  writer.Write("\n/* Begin PBXProject section */\n");

  WritePBXElementPrologue(writer,2,false,project.pbx_project_guid_, "Project object");
  WritePBXProperty(writer,3,"isa","PBXProject");

  // attributes
  WritePBXElementPrologue(writer,3,false,"attributes");
  WritePBXProperty(writer,4,"LastUpgradeCheck","0720");
  WritePBXProperty(writer,4,"ORGANIZATIONNAME",project.makefile_->organization_);

  // attributes.TargetAttributes
  WritePBXElementPrologue(writer,4,false,"TargetAttributes");
  WritePBXElementPrologue(writer,5,false,project.pbx_native_target_guid_);
  WritePBXProperty(writer,6,"CreatedOnToolsVersion","6.3.2");
  WritePBXElementEpilogue(writer,5);
  WritePBXElementEpilogue(writer,4);
  WritePBXElementEpilogue(writer,3);

  WritePBXProperty(writer,3,"buildConfigurationList",project.pbx_project_xconfiguration_list_guid_,String::Format("Build configuration list for PBXProject \"%s\"", project.makefile_->name_.ToCString()));
  WritePBXProperty(writer,3,"compatibilityVersion", "Xcode 3.2");
  WritePBXProperty(writer,3,"developmentRegion","English");
  WritePBXProperty(writer,3,"hasScannedForEncodings","0");
      
  WritePBXListPropertyPrologue(writer,3,"knownRegions");
  WritePBXListProperty(writer,4,"en");
  WritePBXListPropertyEpilogue(writer,3);

  WritePBXProperty(writer,3,"mainGroup",project.main_pbx_group_guid_);
  WritePBXProperty(writer,3,"productRefGroup",project.products_pbx_group_guid_, "Products");
  WritePBXProperty(writer,3,"projectDirPath","");

  // References...
  if(!project.references_.empty()) {
    WritePBXListPropertyPrologue(writer,3,"projectReferences");
    for(noz_uint32 i=0,c=project.references_.size(); i<c; i++) {
      const Reference& ref = *project.references_[i];

      WritePBXElementPrologue(writer,4,false,"");
      WritePBXProperty(writer,5,"ProductGroup",ref.pbx_group_id_, "Products");
      WritePBXProperty(writer,5,"ProjectRef",ref.file_ref_->guid_, String::Format("%s.xcodeproj", ref.makefile_->name_.ToCString()));

      writer.Write("\t\t\t\t},\n");
    }
    WritePBXListPropertyEpilogue(writer,3);
  }

  // projectRoot
  WritePBXProperty(writer,3,"projectRoot", "");

  // targets
  WritePBXListPropertyPrologue(writer,3,"targets");
  WritePBXListProperty(writer,4,project.pbx_native_target_guid_, project.makefile_->name_);
  WritePBXListPropertyEpilogue(writer,3);

  // Footer
  WritePBXElementEpilogue(writer,2);

  writer.Write("/* End PBXProject section */\n");

  return true;
}

bool XCodeProjectGen::WritePBXReferenceProxySection (Project& project, TextWriter& writer) {
  if(project.references_.empty()) return true;

  writer.Write("\n/* Begin PBXReferenceProxy section */\n");

  for(noz_uint32 i=0,c=project.references_.size(); i<c; i++) {
    const Reference& ref = *project.references_[i];

    WritePBXElementPrologue(writer,2,false,ref.pbx_reference_proxy_guid_, ref.makefile_->xcode_output_filename_);
    WritePBXProperty(writer,3,"isa","PBXReferenceProxy");
    WritePBXProperty(writer,3,"fileType","archive.ar");
    WritePBXProperty(writer,3,"path",ref.makefile_->xcode_output_filename_);
    WritePBXProperty(writer,3,"remoteRef",ref.pbx_container_item_proxy_2_guid_, "PBXContainerItemProxy");
    WritePBXProperty(writer,3,"sourceTree", "BUILT_PRODUCTS_DIR");
    WritePBXElementEpilogue(writer,2);
  }

  writer.Write("/* End PBXReferenceProxy section */\n");
      
  return true;
}

bool XCodeProjectGen::WritePBXResourcesBuildPhaseSection (Project& project, TextWriter& writer) {
  if(!project.has_resources_) return true;

  writer.Write("\n/* Begin PBXResourcesBuildPhase section */\n");

  WritePBXElementPrologue(writer,2,false,project.pbx_resources_build_phase_guid_, "Resources");
  WritePBXProperty(writer,3,"isa","PBXResourcesBuildPhase");
  WritePBXProperty(writer,3,"buildActionMask","2147483647");
  WritePBXListPropertyPrologue(writer,3,"files");

  for(noz_uint32 i=0,c=project.build_files_.size(); i<c; i++) {
    const BuildFile& build_file = *project.build_files_[i];
    if(build_file.phase_ != BuildPhase::Resources) continue;
    WritePBXListProperty(writer,4,build_file.guid_, String::Format("%s in Resources", build_file.file_reference_->name_.ToCString()));
  }

  WritePBXListPropertyEpilogue(writer,3);
  WritePBXProperty(writer,3,"runOnlyForDeploymentPostprocessing","0");
  WritePBXElementEpilogue(writer,2);

  writer.Write("\n/* End PBXResourcesBuildPhase section */\n");

  return true;
}

bool XCodeProjectGen::WritePBXShellScriptBuildPhaseSection (Project& project, TextWriter& writer) {
  if(!project.makefile_->glue_) return true;

  writer.Write("\n/* Begin PBXShellScriptBuildPhase section */\n");

  String exe = Path::GetUnixPath(Path::GetRelativePath(Path::Canonical(Environment::GetExecutablePath()),project.makefile_->target_dir_));
  String makefile_path = Path::GetRelativePath(Path::Canonical(project.makefile_->path_),project.makefile_->target_dir_);
  makefile_path = Path::GetUnixPath(makefile_path);

  WritePBXElementPrologue(writer,2,false,project.glue_pbx_shell_script_build_phase_guid_, "ShellScript");
  WritePBXProperty(writer,3,"isa","PBXShellScriptBuildPhase");
  WritePBXProperty(writer,3,"buildActionMask","2147483647");
  WritePBXListPropertyPrologue(writer,3,"files");
  WritePBXListPropertyEpilogue(writer,3);
  WritePBXListPropertyPrologue(writer,3,"inputPaths");
  WritePBXListPropertyEpilogue(writer,3);
  WritePBXListPropertyPrologue(writer,3,"outputPaths");
  WritePBXListPropertyEpilogue(writer,3);
  WritePBXProperty(writer,3,"runOnlyForDeploymentPostprocessing","0");
  WritePBXProperty(writer,3,"shellPath","/bin/sh");

  StringBuilder script;
  script.Append(exe);
  script.Append(" glue ");
  script.Append(makefile_path);
  script.Append(" -platform=" );
  script.Append(Makefile::PlatformTypeToString(project.makefile_->platform_));
  script.Append(" -config=$CONFIGURATION");
  WritePBXProperty(writer,3,"shellScript",script.ToString());
  WritePBXElementEpilogue(writer,2);

  writer.Write("\n/* End PBXShellScriptBuildPhase section */\n");

  return true;
}

bool XCodeProjectGen::WritePBXSourcesBuildPhaseSection (Project& project, TextWriter& writer) {
  writer.Write("\n/* Begin PBXSourcesBuildPhase section */\n");

  WritePBXElementPrologue(writer,2,false,project.pbx_sources_build_phase_guid_,"Sources");
  WritePBXProperty(writer,3,"isa","PBXSourcesBuildPhase");
  WritePBXProperty(writer,3,"buildActionMask","2147483647");

  WritePBXListPropertyPrologue(writer,3,"files");
  for(noz_uint32 i=0,c=project.build_files_.size(); i<c; i++) {
    const BuildFile& build_file = *project.build_files_[i];
    if(build_file.phase_ != BuildPhase::Sources) continue;       
    WritePBXListProperty(writer,4,build_file.guid_,String::Format("%s in Sources", build_file.file_reference_->name_.ToCString()));
  }
      
  WritePBXListPropertyEpilogue(writer,3);
  WritePBXProperty(writer,3,"runOnlyForDeploymentPostprocessing","0");
  WritePBXElementEpilogue(writer,2);

  writer.Write("/* End PBXSourcesBuildPhase section */\n");

  return true;
}

bool XCodeProjectGen::WritePBXTargetDependencySection (Project& project, TextWriter& writer) {
  if(project.references_.empty()) return true;

  writer.Write("\n/* Begin PBXTargetDependency section */\n");

  for(noz_uint32 i=0,c=project.references_.size(); i<c; i++) {
    const Reference& ref = *project.references_[i];

    WritePBXElementPrologue(writer,2,false,ref.pbx_target_dependency_guid_, "PBXTargetDependency");
    WritePBXProperty(writer,3,"isa","PBXTargetDependency");
    WritePBXProperty(writer,3,"name",ref.makefile_->name_);
    WritePBXProperty(writer,3,"targetProxy", ref.pbx_container_item_proxy_1_guid_, "PBXContainerItemProxy");
    WritePBXElementEpilogue(writer,2);
  }

  writer.Write("/* End PBXTargetDependency section */\n");

  return true;
}

bool XCodeProjectGen::WriteXCBuildConfigurationSection (Project& project, TextWriter& writer) {
  writer.Write("\n/* Begin XCBuildConfiguration section */\n");
   
  for(noz_uint32 i=0,c=project.makefile_->configurations_.size(); i<c; i++) {
    const Makefile::Configuration& cfg = *project.makefile_->configurations_[i];

    WritePBXElementPrologue(writer,2,false,cfg.xcode_project_guid_, cfg.name_);
    WritePBXProperty(writer,3,"isa","XCBuildConfiguration");
        
    WritePBXElementPrologue(writer,3,false,"buildSettings");
    WritePBXProperty(writer,4,"CLANG_CXX_LANGUAGE_STANDARD", "\"c++0x\"");

    if(project.makefile_->platform_ == Makefile::PlatformType::IOS) {
      WritePBXProperty(writer,4,"CODE_SIGN_IDENTITY","iPhone Developer");
      WritePBXProperty(writer,4,"ENABLE_TESTABILITY", "YES");
    }
        
    if(!cfg.optimizations_) {
      WritePBXProperty(writer,4,"GCC_OPTIMIZATION_LEVEL", "0");

      if(project.makefile_->platform_ == Makefile::PlatformType::IOS) WritePBXProperty(writer,4,"ONLY_ACTIVE_ARCH", "YES");
    }

    WritePBXListPropertyPrologue(writer,4,"GCC_PREPROCESSOR_DEFINITIONS");
    WritePBXListProperty(writer,5,"$(inherited)");
    if(project.makefile_->platform_ == Makefile::PlatformType::IOS) {
      WritePBXListProperty(writer,5,"NOZ_IOS");
    } else {
      WritePBXListProperty(writer,5,"NOZ_OSX");
    }
    for(noz_uint32 ii=0,cc=cfg.preprocessor_definitions_.size(); ii<cc; ii++) {
      WritePBXListProperty(writer,5,cfg.preprocessor_definitions_[ii]);
    }
    WritePBXListPropertyEpilogue(writer,4);

    WritePBXListPropertyPrologue(writer,4,"HEADER_SEARCH_PATHS");
    WritePBXListProperty(writer,5,"$(inherited)");

    for(noz_uint32 ii=0,cc=cfg.include_directories_.size(); ii<cc; ii++) {
      WritePBXListProperty(writer,5,cfg.include_directories_[ii]);
    }
    WritePBXListPropertyEpilogue(writer,4);

    // OBJROOT
    WritePBXProperty(writer,4,"OBJROOT",cfg.output_directory_);

    // SDKROOT
    if(project.makefile_->platform_ == Makefile::PlatformType::IOS) {
      WritePBXProperty(writer,4,"SDKROOT","iphoneos");
    } else {
      WritePBXProperty(writer,4,"SDKROOT","macosx");          
    }

    // SYMROOT
    WritePBXProperty(writer,4,"SYMROOT",cfg.bin_directory_);

/*
    // MACH_O_TYPE
    String target_type = cfg->GetString("TARGET_TPYE");
    if(target_type.Equals("APPLICATION",StringComparison::OrdinalIgnoreCase)) {
      writer.Write("\t\t\t\tMACH_O_TYPE = mh_execute;\n");
    } else if(target_type.Equals("CONSOLE",StringComparison::OrdinalIgnoreCase)) {
      writer.Write("\t\t\t\tMACH_O_TYPE = mh_execute;\n");
    } else if(target_type.Equals("LIB",StringComparison::OrdinalIgnoreCase)) {
      writer.Write("\t\t\t\tMACH_O_TYPE = staticlib;\n");
    }
*/

    // GCC_WARN_INHIBIT_ALL_WARNINGS
    if(!cfg.warnings_) {
      WritePBXProperty(writer,4,"GCC_WARN_INHIBIT_ALL_WARNINGS","YES");
    }

    WritePBXElementEpilogue(writer,3);        

    WritePBXProperty(writer,3,"name",cfg.name_);
    WritePBXElementEpilogue(writer,2);        


    // Native Target Configuration
    WritePBXElementPrologue(writer,2,false,cfg.xcode_native_target_guid_, cfg.name_);
    WritePBXProperty(writer,3,"isa","XCBuildConfiguration");
    WritePBXElementPrologue(writer,3,false,"buildSettings");

    if(project.has_resources_) {
      WritePBXProperty(writer,4,"ASSETCATALOG_COMPILER_APPICON_NAME", "AppIcon");
      WritePBXProperty(writer,4,"ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME", "LaunchImage");
    }

    if(project.makefile_->platform_ == Makefile::PlatformType::IOS) WritePBXProperty(writer,4,"CODE_SIGN_IDENTITY","iPhone Developer");
    WritePBXProperty(writer,4,"GCC_PREPROCESSOR_DEFINITIONS", "$(inherited)");
    WritePBXProperty(writer,4,"HEADER_SEARCH_PATHS", "$(inherited)");
    if(!project.plist_path_.IsEmpty()) {
      WritePBXProperty(writer,4,"INFOPLIST_FILE", Path::Combine("$(SRCROOT)",project.plist_filename_));
    }
    WritePBXProperty(writer,4,"OTHER_LDFLAGS", "-ObjC");
    WritePBXProperty(writer,4,"PRODUCT_NAME", "$(TARGET_NAME)");
    WritePBXProperty(writer,4,"SKIP_INSTALL", "YES");
    WritePBXProperty(writer,4,"TARGETED_DEVICE_FAMILY", "1,2");
    WritePBXProperty(writer,4,"USE_HEADERMAP", "NO");
    WritePBXElementEpilogue(writer,3);
    WritePBXProperty(writer,3,"name", cfg.name_);
    WritePBXElementEpilogue(writer,2);   
  }

  // Footer
  writer.Write("/* End XCBuildConfiguration section */\n");

  return true;
}


bool XCodeProjectGen::WriteBuildConfigurationList (Project& project, TextWriter& writer, const char* comment, const String& guid_buildcfglist, bool native) {
  WritePBXElementPrologue(writer,2,false,guid_buildcfglist,String::Format("Build configuration list for %s \"%s\"", comment, project.makefile_->name_.ToCString()));
  WritePBXProperty(writer,3,"isa","XCConfigurationList");
  WritePBXListPropertyPrologue(writer,3,"buildConfigurations");

  for(noz_uint32 i=0,c=project.makefile_->configurations_.size(); i<c; i++) {
    const Makefile::Configuration& cfg = *project.makefile_->configurations_[i];
    WritePBXListProperty(writer,4, native ? cfg.xcode_native_target_guid_ : cfg.xcode_project_guid_, cfg.name_);
  }
  WritePBXListPropertyEpilogue(writer,3);
  WritePBXProperty(writer,3,"defaultConfigurationIsVisible","0");
  WritePBXProperty(writer,3,"defaultConfigurationName","Debug");
  WritePBXElementEpilogue(writer,2);
      
  return true;
}

bool XCodeProjectGen::WriteXCConfigurationListSection (Project& project, TextWriter& writer) {
  writer.Write("\n/* Begin XCConfigurationList section */\n");

  if(!WriteBuildConfigurationList(project, writer,"PBXProject", project.pbx_project_xconfiguration_list_guid_, false)) return false;
  if(!WriteBuildConfigurationList(project, writer,"PBXNativeTarget", project.pbx_native_target_xconfiguration_list_guid_, true)) return false;

  // Footer
  writer.Write("/* End XCConfigurationList section */\n");

  return true;
}

void XCodeProjectGen::AddFramework (Project& project, const String& name, const String& path) {
  BuildFile* build_file = new BuildFile;
  build_file->guid_ = project.GenerateGUID();
  build_file->phase_ = BuildPhase::Frameworks;
  build_file->name_ = name;
  project.build_files_.push_back(build_file);

  FileReference* file_ref = new FileReference;
  file_ref->guid_ = project.GenerateGUID();
  file_ref->name_ = name;
  file_ref->path_ = path;
  file_ref->explicit_file_type_ = "wrapper.framework";
  file_ref->include_in_index_ = 0;
  file_ref->source_tree_ = "SDKROOT";
  build_file->file_reference_ = file_ref;
  build_file->file_reference_guid_ = file_ref->guid_;
  project.file_references_.push_back(file_ref);
}

XCodeProjectGen::Group* XCodeProjectGen::AddGroup (Project& project, const String& name) {
  auto it = project.groups_by_name_.find(name);
  if(it != project.groups_by_name_.end()) return it->second;

  String parent_name = Path::GetDirectoryName(name);
  
  Group* parent_group = project.main_group_;
  if(!parent_name.IsEmpty()) {
    parent_group = AddGroup (project, parent_name);
  }
  noz_assert(parent_group);

  Group* group = new Group;
  group->name_ = Path::GetFilename(name);
  group->guid_ = project.GenerateGUID();
  parent_group->groups_.push_back(group);
  project.groups_.push_back(group);
  project.groups_by_name_[name] = group;

  return group;
}

bool XCodeProjectGen::WritePLIST (Project& project) {
  if(project.plist_path_.IsEmpty()) return true;

  // Open the target stream
  FileStream stream;
  if(!stream.Open(project.plist_path_, FileMode::Truncate)) {
    return false;
  }
      
  StreamWriter writer(&stream);
      
  writer.Write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  writer.Write("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n");
  writer.Write("<plist version=\"1.0\">\n");
  writer.Write("<dict>\n");
  writer.Write("  <key>CFBundleDevelopmentRegion</key>\n");
  writer.Write("  <string>en</string>\n");
  writer.Write("  <key>CFBundleDisplayName</key>\n");
  writer.Write("  <string>${PRODUCT_NAME}</string>\n");
  writer.Write("  <key>CFBundleExecutable</key>\n");
  writer.Write("  <string>${EXECUTABLE_NAME}</string>\n");
  writer.Write("  <key>CFBundleIdentifier</key>\n");
  writer.Write("  <string>");
  writer.Write(project.makefile_->bundle_id_);
  writer.Write("</string>\n");
  writer.Write("  <key>CFBundleInfoDictionaryVersion</key>\n");
  writer.Write("  <string>6.0</string>\n");
  writer.Write("  <key>CFBundleName</key>\n");
  writer.Write("  <string>${PRODUCT_NAME}</string>\n");
  writer.Write("  <key>CFBundlePackageType</key>\n");
  writer.Write("  <string>APPL</string>\n");
  writer.Write("  <key>CFBundleShortVersionString</key>\n");
  writer.Write("  <string>1.0</string>\n");
  writer.Write("  <key>CFBundleSignature</key>\n");
  writer.Write("  <string>\?\?\?\?</string>\n");
  writer.Write("  <key>CFBundleVersion</key>\n");
  writer.Write("  <string>1.0</string>\n");
  writer.Write("  <key>LSRequiresIPhoneOS</key>\n");
  writer.Write("  <true/>\n");
  writer.Write("  <key>UIRequiredDeviceCapabilities</key>\n");
  writer.Write("  <array>\n");
  writer.Write("    <string>armv7</string>\n");
  writer.Write("  </array>\n");
  writer.Write("  <key>UIRequiresFullScreen</key>\n");
  writer.Write("  <true/>\n");
  writer.Write("  <key>UIStatusBarHidden</key>\n");
  writer.Write("  <true/>\n");
  writer.Write("  <key>UIStatusBarStyle</key>\n");
  writer.Write("  <string>UIStatusBarStyleDefault</string>\n");
  writer.Write("  <key>UISupportedInterfaceOrientations</key>\n");
  writer.Write("  <array>\n");
  writer.Write("    <string>UIInterfaceOrientationPortrait</string>\n");
  writer.Write("  </array>\n");
  writer.Write("  <key>UISupportedInterfaceOrientations~ipad</key>\n");
  writer.Write("  <array>\n");
  writer.Write("    <string>UIInterfaceOrientationPortrait</string>\n");
  writer.Write("  </array>\n");
  writer.Write("</dict>\n");
  writer.Write("</plist>\n");
  return true;      
}

bool XCodeProjectGen::GenerateOverride (Makefile* makefile, const String& target_dir) {
  Project project;
  project.next_guid_ = 1;
  project.target_filename_ = String::Format("%s.xcodeproj", makefile->name_.ToCString());
  project.target_path_ = Path::GetUnixPath(Path::Canonical(Path::Combine(target_dir, project.target_filename_)));
  project.makefile_ = makefile;

  // Open the target stream
  FileStream fs;
  if(!fs.Open(Path::Combine(project.target_path_,"project.pbxproj"), FileMode::Truncate)) return false;

  // Generate guids
  project.pbx_project_guid_ = project.GenerateGUID();
  project.main_pbx_group_guid_ = project.GenerateGUID();
  project.products_pbx_group_guid_ = project.GenerateGUID();
  project.reference_products_pbx_group_guid_ = project.GenerateGUID();
  project.pbx_native_target_xconfiguration_list_guid_ = project.GenerateGUID();
  project.pbx_frameworks_build_phase_guid_ = project.GenerateGUID();
  project.pbx_sources_build_phase_guid_ = project.GenerateGUID();
  project.pbx_resources_build_phase_guid_ = project.GenerateGUID();
  project.pbx_native_target_guid_ = project.GenerateGUID();
  project.product_pbx_file_reference_guid_ = project.GenerateGUID();
  project.pbx_project_xconfiguration_list_guid_ = project.GenerateGUID();
  if(makefile->glue_) project.glue_pbx_shell_script_build_phase_guid_ = project.GenerateGUID();

  makefile->xcode_product_guid_ = project.product_pbx_file_reference_guid_;
  makefile->xcode_native_target_guid_ = project.pbx_native_target_guid_;

  // Adjust the target name depending on the TARGET_TYPE
  if(makefile->target_type_ == Makefile::TargetType::Library) {
    makefile->xcode_output_filename_ = String::Format("lib%s.a", makefile->name_.ToCString());
  } else {
    makefile->xcode_output_filename_ = String::Format("%s.app", makefile->name_.ToCString());
  }

  // Pre-process all files...
  for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
    const Makefile::BuildFile& file = *makefile->files_[i];

    String ext = Path::GetExtension(file.path_);
      
    // Files to skip...
    if(ext.Equals(".a",StringComparison::OrdinalIgnoreCase) ||
       ext.Equals(".pch",StringComparison::OrdinalIgnoreCase) ||
       ext.Equals(".framework",StringComparison::OrdinalIgnoreCase) ||
       ext.Equals(".xcodeproj",StringComparison::OrdinalIgnoreCase) ||
       ext.Equals(".plist",StringComparison::OrdinalIgnoreCase) ) {
      continue;
    }

    // .XCASSETS
    if(ext.Equals(".xcassets", StringComparison::OrdinalIgnoreCase)) {
      BuildFile* build_file = new BuildFile;
      build_file->guid_ = project.GenerateGUID();
      build_file->phase_ = BuildPhase::Resources;
      build_file->name_ = Path::GetFilename(file.path_);
      project.build_files_.push_back(build_file);
      project.has_resources_ = true;

      FileReference* file_ref = new FileReference;
      file_ref->guid_ = project.GenerateGUID();
      file_ref->last_known_file_type_ = "folder.assetcatalog";
      file_ref->path_ = Path::GetUnixPath(file.path_);
      file_ref->name_ = Path::GetFilename(file_ref->path_);
      file_ref->source_tree_ = "<group>";
      file_ref->group_ = file.group_;
      project.file_references_.push_back(file_ref);
      build_file->file_reference_guid_ = file_ref->guid_;
      build_file->file_reference_ = file_ref;
      continue;
    }

    // .nozsettings  .nozpak 
    if(ext.Equals(".nozsettings", StringComparison::OrdinalIgnoreCase) || ext.Equals(".nozpak", StringComparison::OrdinalIgnoreCase)) {
      BuildFile* build_file = new BuildFile;
      build_file->guid_ = project.GenerateGUID();
      build_file->phase_ = BuildPhase::Resources;
      build_file->name_ = Path::GetFilename(file.path_);
      project.build_files_.push_back(build_file);
      project.has_resources_ = true;

      FileReference* file_ref = new FileReference;
      file_ref->guid_ = project.GenerateGUID();
      file_ref->last_known_file_type_ = ext.Equals(".nozpak", StringComparison::OrdinalIgnoreCase) ? "file" : "text";
      file_ref->path_ = Path::GetUnixPath(file.path_);
      file_ref->name_ = Path::GetFilename(file_ref->path_);
      file_ref->source_tree_ = "<group>";
      file_ref->group_ = file.group_;
      project.file_references_.push_back(file_ref);
      build_file->file_reference_guid_ = file_ref->guid_;
      build_file->file_reference_ = file_ref;
      continue;
    }

    if(ext.Equals(".h",StringComparison::OrdinalIgnoreCase)) {
      FileReference* file_ref = new FileReference;
      file_ref->guid_ = project.GenerateGUID();
      file_ref->path_ = Path::GetUnixPath(file.path_);
      file_ref->name_ = Path::GetFilename(file_ref->path_);
      file_ref->source_tree_ = "<group>";
      file_ref->file_encoding_ = 4;
      file_ref->group_ = file.group_;
      project.file_references_.push_back(file_ref);

      if(ext.Equals(".h",StringComparison::OrdinalIgnoreCase)) {
        file_ref->last_known_file_type_ = "sourcecode.c.h";
      }
      continue;
    }

    // .CPP  .C  .M  .MM
    if(ext.Equals(".cpp",StringComparison::OrdinalIgnoreCase) ||
       ext.Equals(".c",StringComparison::OrdinalIgnoreCase) ||
       ext.Equals(".mm",StringComparison::OrdinalIgnoreCase) ) {

      BuildFile* build_file = new BuildFile;
      build_file->guid_ = project.GenerateGUID();
      build_file->phase_ = BuildPhase::Sources;
      build_file->name_ = Path::GetFilename(file.path_);
      project.build_files_.push_back(build_file);

      FileReference* file_ref = new FileReference;
      file_ref->guid_ = project.GenerateGUID();
      file_ref->path_ = Path::GetUnixPath(file.path_);
      file_ref->name_ = Path::GetFilename(file_ref->path_);
      file_ref->source_tree_ = "<group>";
      file_ref->file_encoding_ = 4;
      file_ref->group_ = file.group_;
      project.file_references_.push_back(file_ref);
      build_file->file_reference_ = file_ref;
      build_file->file_reference_guid_ = file_ref->guid_;

      if(ext.Equals(".h",StringComparison::OrdinalIgnoreCase)) {
        file_ref->last_known_file_type_ = "sourcecode.c.h";
      } else if(ext.Equals(".cpp",StringComparison::OrdinalIgnoreCase)) {
        file_ref->last_known_file_type_ = "sourcecode.c.cpp";
      } else if(ext.Equals(".c",StringComparison::OrdinalIgnoreCase)) {
        file_ref->last_known_file_type_ = "sourcecode.c.c";
      } else if(ext.Equals(".mm", StringComparison::OrdinalIgnoreCase)) {
        file_ref->last_known_file_type_ = "sourcecode.cpp.objcpp";
      }

      continue;
    }    
  }

  // Asset directories..
  for(noz_uint32 i=0,c=makefile->assets_directories_.size(); i<c; i++) {
    const String& path = makefile->assets_directories_[i];

    String dstPath;
    if(path.Equals("../../Assets")) {
      dstPath = "";
    } else {
      dstPath = "noz";
    }

    String full_path = Path::Canonical(Path::Combine(makefile->target_dir_,path));

    BuildFile* build_file = new BuildFile;
    build_file->guid_ = project.GenerateGUID();
    build_file->phase_ = BuildPhase::CopyFiles;
    build_file->name_ = Path::GetFilename(full_path);
    project.build_files_.push_back(build_file);

    FileReference* file_ref = new FileReference;
    file_ref->guid_ = project.GenerateGUID();
    file_ref->last_known_file_type_ = "folder";
    file_ref->path_ = Path::GetUnixPath(Path::GetRelativePath(full_path,makefile->target_dir_));
    file_ref->name_ = Path::GetFilename(file_ref->path_);
    file_ref->source_tree_ = "<group>";
    project.file_references_.push_back(file_ref);
    build_file->file_reference_ = file_ref;
    build_file->file_reference_guid_ = file_ref->guid_;

    CopyFilesBuildPhase* phase = new CopyFilesBuildPhase;
    phase->dst_path_ = dstPath;
    phase->file_guid_ = build_file->guid_;
    phase->guid_ = project.GenerateGUID();
    project.copy_file_build_phases_.push_back(phase);
  }

  // PLIST
  if(makefile->platform_ == Makefile::PlatformType::IOS && !makefile->bundle_id_.IsEmpty()) {
    project.plist_filename_ = String::Format("%s.plist", makefile->name_.ToCString());
    project.plist_path_ = Path::Combine(target_dir, project.plist_filename_);

    FileReference* file_ref = new FileReference;
    file_ref->guid_ = project.GenerateGUID();
    file_ref->last_known_file_type_ = "text.plist.xml";
    file_ref->path_ = project.plist_filename_;
    file_ref->source_tree_ = "<group>";
    file_ref->file_encoding_ = 4;
    project.file_references_.push_back(file_ref);
  }

  // Generate a file reference for the product
  FileReference* product_file_ref = new FileReference;
  product_file_ref->guid_ = project.product_pbx_file_reference_guid_;
  product_file_ref->name_ = Path::GetFilenameWithoutExtension(project.makefile_->xcode_output_filename_);
  product_file_ref->path_ = project.makefile_->xcode_output_filename_;
  product_file_ref->source_tree_ = "BUILT_PRODUCTS_DIR";
  product_file_ref->include_in_index_ = 0;
  product_file_ref->explicit_file_type_ = makefile->target_type_ == Makefile::TargetType::Library ? "archive.ar" : "compiled.mach-o.executable";
  product_file_ref->group_ = "*";
  project.file_references_.push_back(product_file_ref);

  // Create the main group
  project.main_group_ = new Group;
  project.main_group_->guid_ = project.main_pbx_group_guid_;
  project.groups_.push_back(project.main_group_);

  // Pre-process references
  for(noz_uint32 i=0,c=makefile->references_.size(); i<c; i++) {
    Makefile* ref_makefile = GetReferenceMakefile(makefile, makefile->references_[i].path_);
    if(nullptr == ref_makefile) continue;

    Reference* reference = new Reference;
    reference->pbx_container_item_proxy_1_guid_ = project.GenerateGUID();
    reference->pbx_container_item_proxy_2_guid_ = project.GenerateGUID();
    reference->pbx_target_dependency_guid_ = project.GenerateGUID();
    reference->pbx_reference_proxy_guid_ = project.GenerateGUID();
    reference->container_portal_guid_ = project.GenerateGUID();
    reference->pbx_group_id_ = project.GenerateGUID();
    reference->makefile_ = ref_makefile;
    project.references_.push_back(reference);

    // Initialize file reference used for product
    reference->product_file_ref_.guid_ = reference->pbx_reference_proxy_guid_;
    reference->product_file_ref_.name_ = ref_makefile->xcode_output_filename_;

    BuildFile* build_file = new BuildFile;
    build_file->guid_ = project.GenerateGUID();;
    build_file->phase_ = BuildPhase::Frameworks;
    build_file->name_ = reference->makefile_->xcode_output_filename_;
    build_file->file_reference_guid_ = reference->pbx_reference_proxy_guid_;
    project.build_files_.push_back(build_file);
    reference->build_file_ = build_file;

    FileReference* file_ref = new FileReference;
    file_ref->guid_ = reference->container_portal_guid_;
    file_ref->path_ = Path::Canonical(Path::Combine(ref_makefile->target_dir_, String::Format("%s.xcodeproj", ref_makefile->name_.ToCString())));
    file_ref->path_ = Path::GetUnixPath(Path::GetRelativePath(file_ref->path_,makefile->target_dir_));
    file_ref->name_ = Path::GetFilename(file_ref->path_);
    file_ref->explicit_file_type_ = "wrapper.pb-project";
    file_ref->source_tree_ = "<group>";
    reference->file_ref_ = file_ref;
    project.file_references_.push_back(file_ref);

    // Create the products group
    Group* products_group = new Group;
    products_group->name_ = "Products";
    products_group->guid_ = reference->pbx_group_id_;
    products_group->file_references_.push_back(&reference->product_file_ref_);
    project.groups_.push_back(products_group);
  }

  // If not a lib add known frameworks
  if(makefile->target_type_ != Makefile::TargetType::Library) {
    if(makefile->platform_ == Makefile::PlatformType::IOS) {
      AddFramework(project, "UIKit.framework", "System/Library/Frameworks/UIKit.framework");
      AddFramework(project, "CoreGraphics.framework", "System/Library/Frameworks/CoreGraphics.framework");
      AddFramework(project, "Foundation.framework", "System/Library/Frameworks/Foundation.framework");
      AddFramework(project, "GLKit.framework", "System/Library/Frameworks/GLKit.framework");
      AddFramework(project, "QuartzCore.framework", "System/Library/Frameworks/QuartzCore.framework");
      AddFramework(project, "CoreFoundation.framework", "System/Library/Frameworks/CoreFoundation.framework");
      AddFramework(project, "OpenGLES.framework", "System/Library/Frameworks/OpenGLES.framework");
      AddFramework(project, "OpenAL.framework", "System/Library/Frameworks/OpenAL.framework");
    } else {
      AddFramework(project, "AppKit.framework", "System/Library/Frameworks/AppKit.framework");
      AddFramework(project, "Cocoa.framework", "System/Library/Frameworks/Cocoa.framework");
      AddFramework(project, "CoreVideo.framework", "System/Library/Frameworks/CoreVideo.framework");
      AddFramework(project, "OpenGL.framework", "System/Library/Frameworks/OpenGL.framework");
    }
  }

  // Generate file groups 
  for(noz_uint32 i=0,c=project.file_references_.size(); i<c; i++) {
    FileReference& file_ref = *project.file_references_[i];

    if(file_ref.group_.Equals("*")) continue;

    // If no group is given add directly to the main group..
    if(file_ref.group_.IsEmpty()) {
      project.main_group_->file_references_.push_back(&file_ref);
      continue;
    }

    AddGroup(project, file_ref.group_)->file_references_.push_back(&file_ref);      
  }

  // Create the products group
  Group* products_group = new Group;
  products_group->name_ = "Products";
  products_group->guid_ = project.products_pbx_group_guid_;
  products_group->file_references_.push_back(product_file_ref);
  project.main_group_->groups_.push_back(products_group);  
  project.groups_.push_back(products_group);

  // Generate GUIDS for configurations
  for(noz_uint32 i=0,c=project.makefile_->configurations_.size(); i<c; i++) {
    Makefile::Configuration& cfg = *project.makefile_->configurations_[i];
    cfg.xcode_native_target_guid_ = project.GenerateGUID();
    cfg.xcode_project_guid_ = project.GenerateGUID();
  }

  StreamWriter writer(&fs);

  writer.WriteLine("// !$*UTF8*$!");
  writer.WriteLine("{");
  writer.WriteLine("\tarchiveVersion = 1;");
  writer.WriteLine("\tclasses = {");
  writer.WriteLine("\t};");
  writer.WriteLine("\tobjectVersion = 46;");
  writer.WriteLine("\tobjects = {");
  writer.WriteLine("");

  if(!WritePBXBuildFileSection(project,writer)) return false;
  if(!WritePBXContainerItemProxySection(project,writer)) return false;
  if(!WritePBXCopyFilesBuildPhaseSection(project,writer)) return false;
  if(!WritePBXFileReferenceSection(project, writer)) return false;
  if(!WritePBXFrameworksBuildPhaseSection(project, writer)) return false;
  if(!WritePBXGroupSection(project,writer)) return false;
  if(!WritePBXNativeTargetSection(project,writer)) return false;
  if(!WritePBXProjectSection(project,writer)) return false;
  if(!WritePBXReferenceProxySection(project,writer)) return false;
  if(!WritePBXResourcesBuildPhaseSection(project, writer)) return false;
  if(!WritePBXShellScriptBuildPhaseSection(project,writer)) return false;
  if(!WritePBXSourcesBuildPhaseSection(project,writer)) return false;
  if(!WritePBXTargetDependencySection(project,writer)) return false;
  if(!WriteXCBuildConfigurationSection(project,writer)) return false;
  if(!WriteXCConfigurationListSection(project,writer)) return false;

  // objects end
  writer.Write("\t};\n");

  // rootObject
  writer.Write("\trootObject = ");
  writer.Write(project.pbx_project_guid_);
  writer.Write(" /* Project object */;\n");

  // Footer
  writer.Write("}\n");

  return WritePLIST(project);
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Guid.h>
#include <noz/Editor/Tool/Makefile.h>
#include <noz/IO/StreamWriter.h>
#include "VisualStudioProjectGen.h"

using namespace noz;

static const char* VisualStudioPlatforms[] = {"Win32", "x64"}; 
static const char* VisualStudioPlatformsName[] = {"x86", "x64"}; 
static noz_uint32 VisualStudioPlatformCount = 2;

VisualStudioProjectGen::ItemType VisualStudioProjectGen::GetItemType (const Makefile::BuildFile& file) const {
  String ext = Path::GetExtension (file.path_);

  // <ClInclude>
  if(!ext.CompareTo(".h") || !ext.CompareTo(".pch")) return ItemType::ClInclude;

  // <ClCompile>
  if(!ext.CompareTo(".cpp") || !ext.CompareTo(".c")) return ItemType::ClCompile;

  // <ClCompile>
  if(!ext.CompareTo(".rc")) return ItemType::ResourceCompile;

  return ItemType::None;
}

String VisualStudioProjectGen::ItemTypeToString (ItemType it) const {
  static const String ItemTypeClInclude ("ClInclude");
  if(it == ItemType::ClInclude) return ItemTypeClInclude;
  static const String ItemTypeClCompile ("ClCompile");
  if(it == ItemType::ClCompile) return ItemTypeClCompile;
  static const String ItemTypeResourceCompile ("ResourceCompile");
  if(it == ItemType::ResourceCompile) return ItemTypeResourceCompile;
  static const String ItemTypeNone ("None");
  return ItemTypeNone;
}

bool VisualStudioProjectGen::IsReferenceOf (Makefile* makefile, Makefile* reference) const {
  for(noz_uint32 i=0,c=makefile->references_.size(); i<c; i++) {
    Makefile* r = GetReferenceMakefile(makefile,makefile->references_[i].path_);
    if(r==reference) return true;
    if(IsReferenceOf(r,reference)) return true;
  }

  return false;
}

bool VisualStudioProjectGen::WriteSolution (Makefile* makefile, const String& target_dir) {
  // Only write the solution if a guid was specified for it.
  if(makefile->solution_guid_.IsEmpty()) return true;

  // Target solution file path
  String sln_path = String::Format("%s.sln", Path::Combine(target_dir,makefile->name_).ToCString());

  // Open the target stream
  FileStream fs;
  if(!fs.Open(sln_path, FileMode::Truncate)) return false;

  StreamWriter writer(&fs);

  writer.Write("Microsoft Visual Studio Solution File, Format Version 12.00\r\n");
  writer.Write("# Visual Studio 14\r\n");
  writer.Write("VisualStudioVersion = 14.0.23107.0\r\n");
  writer.Write("MinimumVisualStudioVersion = 10.0.40219.1\r\n");
  writer.Write("Project(\"{");
  writer.Write(makefile->solution_guid_.ToString());
  writer.Write("}\") = \"");
  writer.Write(makefile->name_);
  writer.Write("\", \"");
  writer.Write(makefile->name_);
  writer.Write(".vcxproj\", \"{");
  writer.Write(makefile->guid_.ToString());
  writer.Write("}\"\r\n");

  std::vector<Makefile*> reference_makefiles;
  for(noz_uint32 i=0,c=references_.size(); i<c; i++) {
    if(references_[i] == makefile) continue;
    if(IsReferenceOf(makefile,references_[i])) {
      reference_makefiles.push_back(references_[i]);
    }
  }

  if(reference_makefiles.size() > 1) {
    writer.Write("\tProjectSection(ProjectDependencies) = postProject\r\n");
    for(auto it=reference_makefiles.begin(); it!=reference_makefiles.end(); it++) {
      Makefile* reference_makefile = *it;
      String ref_guid = reference_makefile->guid_.ToString();
      writer.Write("\t\t{");
      writer.Write(ref_guid);
      writer.Write("} = {");
      writer.Write(ref_guid);
      writer.Write("}\r\n");
    }        
    writer.Write("\tEndProjectSection\r\n");
  }

  writer.Write("EndProject\r\n");

  for(auto it=reference_makefiles.begin(); it!=reference_makefiles.end(); it++) {
    Makefile* ref = *it;

    writer.Write("Project(\"{");
    writer.Write(makefile->solution_guid_.ToString());
    writer.Write("}\") = \"");
    writer.Write(ref->name_);
          
    writer.Write("\", \"");

    String path = Path::GetWindowsPath(Path::GetRelativePath(ref->target_path_, target_dir));
    writer.Write(path);
    writer.Write("\", \"{");
    writer.Write(ref->guid_.ToString());
    writer.Write("}\"\r\n");
    writer.Write("EndProject\r\n");
  }

  writer.Write("Global\r\n");
  writer.Write("\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\r\n");
 
  for(auto itcfg=makefile->configurations_.begin(); itcfg!=makefile->configurations_.end(); itcfg++) {
    const Makefile::Configuration& cfg = **itcfg;      
    for(noz_uint32 i=0; i<VisualStudioPlatformCount; i++) {
      writer.Write("\t\t");
      writer.Write(cfg.name_);
      writer.Write("|");
      writer.Write(VisualStudioPlatformsName[i]);
      writer.Write(" = ");
      writer.Write(cfg.name_);
      writer.Write("|");
      writer.Write(VisualStudioPlatformsName[i]);
      writer.Write("\r\n");
    }
  }

  writer.Write("\tEndGlobalSection\r\n");

  writer.Write("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\r\n");

  // Add ourself for final step.
  reference_makefiles.push_back(makefile);

  for(auto it=reference_makefiles.begin(); it!=reference_makefiles.end(); it++) {
    Makefile* reference_makefile = *it;

    for(auto itcfg=makefile->configurations_.begin(); itcfg!=makefile->configurations_.end(); itcfg++) {
      const Makefile::Configuration& cfg = **itcfg;      

      for(noz_uint32 p=0; p<VisualStudioPlatformCount; p++) {
        writer.Write("\t\t{");
        writer.Write(reference_makefile->guid_.ToString());
        writer.Write("}.");
        writer.Write(cfg.name_);
        writer.Write("|");
        writer.Write(VisualStudioPlatformsName[p]);
        writer.Write(".ActiveCfg = ");
        writer.Write(cfg.name_);
        writer.Write("|");
        writer.Write(VisualStudioPlatforms[p]);
        writer.Write("\r\n");

        writer.Write("\t\t{");
        writer.Write(reference_makefile->guid_.ToString());
        writer.Write("}.");

        writer.Write(cfg.name_);
        writer.Write("|");
        writer.Write(VisualStudioPlatformsName[p]);
        writer.Write(".Build.0 = ");
        writer.Write(cfg.name_);
        writer.Write("|");
        writer.Write(VisualStudioPlatforms[p]);
        writer.Write("\r\n");
      }
    }
  }

  writer.Write("\tEndGlobalSection\r\n");

	writer.Write("\tGlobalSection(SolutionProperties) = preSolution\r\n");
	writer.Write("\t\tHideSolutionNode = FALSE\r\n");
	writer.Write("\tEndGlobalSection\r\n");
  writer.Write("EndGlobal\r\n");      

  return true;
}

void VisualStudioProjectGen::WriteFilters (Makefile* makefile, TextWriter& writer, ItemType item_type) {
  writer.Write("  <ItemGroup>\r\n");
  for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
    Makefile::BuildFile& file = *makefile->files_[i];

    if(GetItemType(file) != item_type) continue;

    writer.Write("    <");
    writer.Write(ItemTypeToString(item_type));
    writer.Write(" Include=\"");
    writer.Write(file.path_);
    writer.Write("\"");

    if(!file.group_.IsEmpty()) {
      writer.Write(">\r\n");
      writer.Write("      <Filter>");
      writer.Write(file.group_);
      writer.Write("</Filter>\r\n");
      writer.Write("    </");
      writer.Write(ItemTypeToString(item_type));
      writer.Write(">\r\n");
    } else {
      writer.Write("/>\r\n");
    }
  }
  writer.Write("  </ItemGroup>\r\n");
}

bool VisualStudioProjectGen::WriteFilters (Makefile* makefile, const String& target_dir) {
  // Target filters file path
  String target_path = String::Format("%s.vcxproj.filters", Path::Combine(target_dir, makefile->name_).ToCString());

  // Open the target stream
  FileStream fs;
  if(!fs.Open(target_path, FileMode::Truncate)) return false;

  StreamWriter writer(&fs);

  writer.Write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
  writer.Write("<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n");

  std::set<String> groups;

  for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
    Makefile::BuildFile& file = *makefile->files_[i];
    String group = file.group_;
    if(!group.IsEmpty()) {
      group = Path::GetWindowsPath(group);
      file.group_ = group;
    }
    while (!group.IsEmpty()) {
      groups.insert(group);
      group = Path::GetDirectoryName(group);
    }
  }

  WriteFilters(makefile,writer,ItemType::ClCompile);
  WriteFilters(makefile,writer,ItemType::ClInclude);
  WriteFilters(makefile,writer,ItemType::None);
  WriteFilters(makefile,writer,ItemType::ResourceCompile);

  // Write groups.
  if(!groups.empty()) {
    noz_uint64 next_guid = 1;
    writer.Write("  <ItemGroup>\r\n");
    for(auto itg=groups.begin(); itg!=groups.end(); itg++) {
      writer.Write("    <Filter Include=\"");
      writer.Write((*itg));
      writer.Write("\">\r\n");
      writer.Write("      <UniqueIdentifier>{");
      writer.Write(Guid(0,next_guid++).ToString());
      writer.Write("}</UniqueIdentifier>\r\n");
      writer.Write("    </Filter>\r\n");          
    }
    writer.Write("  </ItemGroup>\r\n");
  }

  writer.Write("</Project>\r\n");

  return true;
}

bool VisualStudioProjectGen::GenerateOverride (Makefile* makefile, const String& target_dir) {
  // Create target visual studio project path
  makefile->target_path_ = String::Format("%s.vcxproj", Path::Combine(target_dir, makefile->name_).ToCString());

  // Generate a guid for the makefile if it doesnt have one.
  if(makefile->guid_.IsEmpty()) makefile->guid_ = Guid::Generate();

  // Open the target stream
  FileStream fs;
  if(!fs.Open(makefile->target_path_, FileMode::Truncate)) return false;

  StreamWriter writer(&fs);

  writer.Write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n");
  writer.Write("<Project DefaultTargets=\"Build\" ToolsVersion=\"14.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\r\n");

  writer.Write("  <ItemGroup Label=\"ProjectConfigurations\">\r\n");
  for(auto itcfg=makefile->configurations_.begin(); itcfg!=makefile->configurations_.end(); itcfg++) {
    const Makefile::Configuration& cfg = **itcfg;
    for(noz_uint32 p=0;p<VisualStudioPlatformCount;p++) {
      writer.Write("    <ProjectConfiguration Include=\"");
      writer.Write(cfg.name_);
      writer.Write("|");
      writer.Write(VisualStudioPlatforms[p]);
      writer.Write("\">\r\n");
      writer.Write("      <Configuration>");
      writer.Write(cfg.name_);
      writer.Write("</Configuration>\r\n");
      writer.Write("      <Platform>");
      writer.Write(VisualStudioPlatforms[p]);
      writer.Write("</Platform>\r\n");
      writer.Write("    </ProjectConfiguration>\r\n");
    }
  }

  writer.Write("  </ItemGroup>\r\n");

  // Write "Globals" block
  writer.Write("  <PropertyGroup Label=\"Globals\">\r\n");
  writer.Write("    <ProjectGuid>{");

  writer.Write(makefile->guid_.ToString());
  writer.Write("}</ProjectGuid>\r\n");
  writer.Write("    <Keyword>Win32Proj</Keyword>\r\n");
  writer.Write("    <RootNamespace>");
  //writer.Write(makefile->GetString("TARGET_NAME"));
  writer.Write("</RootNamespace>\r\n");
  writer.Write("    <WindowsTargetPlatformVersion>8.1</WindowsTargetPlatformVersion>\r\n");
  writer.Write("  </PropertyGroup>\r\n");

  // Import default C++ properites
  writer.Write("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\r\n");

  // Write per configuration properties
  for(auto itcfg=makefile->configurations_.begin(); itcfg!=makefile->configurations_.end(); itcfg++) {
    const Makefile::Configuration& cfg = **itcfg;
    for(noz_uint32 p=0;p<VisualStudioPlatformCount;p++) {
      writer.Write("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='");
      writer.Write(cfg.name_);
      writer.Write("|");
      writer.Write(VisualStudioPlatforms[p]);
      writer.Write("'\" Label=\"Configuration\">\r\n");

      if(cfg.optimizations_) {
        writer.Write("    <UseDebugLibraries>false</UseDebugLibraries>\r\n");
        writer.Write("    <WholeProgramOptimization>true</WholeProgramOptimization>\r\n");
      } else {
        writer.Write("    <UseDebugLibraries>true</UseDebugLibraries>\r\n");
      }

      switch(makefile->target_type_) {
        case Makefile::TargetType::Library:
          writer.Write("    <ConfigurationType>StaticLibrary</ConfigurationType>\r\n");
          break;
          
        case Makefile::TargetType::Application:
          writer.Write("    <ConfigurationType>Application</ConfigurationType>\r\n");
          break;

        case Makefile::TargetType::Console:
          writer.Write("    <ConfigurationType>Application</ConfigurationType>\r\n");
          break;
      }

      writer.Write("    <PlatformToolset>v140</PlatformToolset>\r\n");
      writer.Write("    <CharacterSet>MultiByte</CharacterSet>\r\n");
      writer.Write("  </PropertyGroup>\r\n");
    }
  }

  // Import C++ properties
  writer.Write("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\r\n");

  // Import user properties
  for(auto itcfg=makefile->configurations_.begin(); itcfg!=makefile->configurations_.end(); itcfg++) {
    const Makefile::Configuration& cfg = **itcfg;
    for(noz_uint32 p=0; p<VisualStudioPlatformCount;p++) {
      writer.Write("  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='");
      writer.Write(cfg.name_);
      writer.Write("|");
      writer.Write(VisualStudioPlatforms[p]);
      writer.Write("'\">\r\n");
      writer.Write("    <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\r\n");
      writer.Write("  </ImportGroup>\r\n");
    }
  }

  // User macros
  writer.Write("  <PropertyGroup Label=\"UserMacros\" />\r\n");

  // Configuration output and intermediate directories
  for(auto itcfg=makefile->configurations_.begin(); itcfg!=makefile->configurations_.end(); itcfg++) {
    const Makefile::Configuration& cfg = **itcfg;
    for(noz_uint32 p=0; p<VisualStudioPlatformCount;p++) {
      writer.Write("  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='");
      writer.Write(cfg.name_);
      writer.Write("|");
      writer.Write(VisualStudioPlatforms[p]);
      writer.Write("'\">\r\n");

      if(cfg.optimizations_) {
        writer.Write ("    <LinkIncremental>false</LinkIncremental>\r\n");
      } else {
        writer.Write ("    <LinkIncremental>true</LinkIncremental>\r\n");
      }

      writer.Write("    <OutDir>");
      writer.Write(Path::GetWindowsPath(cfg.bin_directory_));
      if(!Path::IsPathSeparator(cfg.bin_directory_[cfg.bin_directory_.GetLength()-1])) {
        writer.Write("\\");
      }
      writer.Write(VisualStudioPlatforms[p]);
      writer.Write("\\");
      writer.Write("</OutDir>\r\n");
      writer.Write("    <IntDir>");
      writer.Write(Path::GetWindowsPath(cfg.output_directory_));
      if(!Path::IsPathSeparator(cfg.output_directory_[cfg.output_directory_.GetLength()-1])) {
        writer.Write("\\");
      }
      writer.Write(VisualStudioPlatforms[p]);
      writer.Write("\\");
      writer.Write("</IntDir>\r\n");
      writer.Write("  </PropertyGroup>\r\n");
    }
  }

  // <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
  for(auto itcfg=makefile->configurations_.begin(); itcfg!=makefile->configurations_.end(); itcfg++) {
    const Makefile::Configuration& cfg = **itcfg;
    for(noz_uint32 p=0; p<VisualStudioPlatformCount;p++) {
      writer.Write("  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='");
      writer.Write(cfg.name_);
      writer.Write("|");
      writer.Write(VisualStudioPlatforms[p]);
      writer.Write("'\">\r\n");

      // <ClCompile>
      writer.Write("    <ClCompile>\r\n");

      // <PrecompiledHeader>
      if(!cfg.precompiled_header_file_.IsEmpty()) {      
        if(cfg.precompiled_header_ == Makefile::PrecompiledHeader::Use) {
          writer.Write("      <PrecompiledHeader>Use</PrecompiledHeader>\r\n");
        } else if(cfg.precompiled_header_ == Makefile::PrecompiledHeader::Create) {
          writer.Write("      <PrecompiledHeader>Create</PrecompiledHeader>\r\n");
        } else {
          writer.Write("      <PrecompiledHeader>NotUsing</PrecompiledHeader>\r\n");
        }
        writer.Write("      <PrecompiledHeaderFile>");
        writer.Write(cfg.precompiled_header_file_);
        writer.Write("</PrecompiledHeaderFile>\r\n");
      }
          
      // <WarningLevel>
      if(cfg.warnings_) {
        writer.Write("      <WarningLevel>Level3</WarningLevel>\r\n");
      } else {
        writer.Write("      <WarningLevel>TurnOffAllWarnings</WarningLevel>\r\n");
      }

      if(!cfg.optimizations_) {
        writer.Write("      <Optimization>Disabled</Optimization>\r\n");
      } else {
        writer.Write("      <Optimization>MaxSpeed</Optimization>\r\n");
        writer.Write("      <FunctionLevelLinking>true</FunctionLevelLinking>\r\n");
        writer.Write("      <IntrinsicFunctions>true</IntrinsicFunctions>\r\n");          
      }          

      // <PreprocessorDefinitions>
      writer.Write("      <PreprocessorDefinitions>");
      for(noz_uint32 i=0,c=cfg.preprocessor_definitions_.size(); i<c; i++) {
        writer.Write(cfg.preprocessor_definitions_[i]);
        writer.Write(";");
      }
      writer.Write("NOZ_WINDOWS; WIN32;_WINDOWS;_MBCS; %(PreprocessorDefinitions)</PreprocessorDefinitions>\r\n");

      // <AdditionalIncludeDirectories>
      if(!cfg.include_directories_.empty()) {
        writer.Write("      <AdditionalIncludeDirectories>");
        for(noz_uint32 i=0,c=cfg.include_directories_.size(); i<c; i++) {
          writer.Write(cfg.include_directories_[i]);
          writer.Write(";");
        }
        writer.Write("</AdditionalIncludeDirectories>\r\n");
      }

      // </ClCompile>
      writer.Write("    </ClCompile>\r\n");

      // <Link>
      writer.Write("    <Link>\r\n");

      // <AdditionalDependencies>
      writer.Write("      <AdditionalDependencies>");
      if(makefile->target_type_ != Makefile::TargetType::Library) {
        writer.Write("opengl32.lib;rpcrt4.lib;ws2_32.lib;winmm.lib;");
      }
      for(noz_uint32 i=0,c=cfg.libraries_.size(); i<c; i++) {
        writer.Write(cfg.libraries_[i]);
        writer.Write(";");
      }
      writer.Write("%(AdditionalDependencies)</AdditionalDependencies>\r\n");

      if(cfg.optimizations_) {
        writer.Write("      <EnableCOMDATFolding>true</EnableCOMDATFolding>\r\n");
        writer.Write("      <OptimizeReferences>true</OptimizeReferences>\r\n");          
      }

      // <GenerateDebugInformation>
      //if(cfg->GetBoolean("DEBUG_INFO",true)) {
      writer.Write("      <GenerateDebugInformation>true</GenerateDebugInformation>\r\n");
      //}

      // <SubSystem>
      switch(makefile->target_type_) {
        default: break;

        case Makefile::TargetType::Application:
          writer.Write("      <SubSystem>Windows</SubSystem>\r\n");
          break;
        case Makefile::TargetType::Console:
          writer.Write("      <SubSystem>Console</SubSystem>\r\n");
          break;
      } 

      // </Link>
      writer.Write("    </Link>\r\n");

      // GLUE script
      if(makefile->glue_) {
        String exe = Path::GetWindowsPath(Path::GetRelativePath(Path::Canonical(Environment::GetExecutablePath()),makefile->target_dir_));
        String makefile_path = Path::GetWindowsPath(Path::GetRelativePath(makefile->path_,makefile->target_dir_));

        writer.Write("    <PreBuildEvent>\r\n");
        writer.Write("      <Command>");
        writer.Write(exe);
        writer.Write(" glue ");
        writer.Write(makefile_path);
        writer.Write(" -platform=windows -config=");
        writer.Write(cfg.name_);
        writer.Write("</Command>\r\n");
        writer.Write("    </PreBuildEvent>\r\n");
      }

      // </ItemDefinitionGroup>
      writer.Write("  </ItemDefinitionGroup>\r\n");
    }
  }

  // <ItemGroup> <ClCompile>
  writer.Write("  <ItemGroup>\r\n");
  for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
    const Makefile::BuildFile& file = *makefile->files_[i];
    if(GetItemType(file) != ItemType::ClCompile) continue;

    writer.Write("    <ClCompile Include=\"");
    writer.Write(file.path_);
    writer.Write("\"");
          
    if(file.precompiled_header_ != Makefile::PrecompiledHeader::Use) {
      writer.Write(">\r\n");
      for(auto itcfg=makefile->configurations_.begin(); itcfg!=makefile->configurations_.end(); itcfg++) {
        const Makefile::Configuration& cfg = **itcfg;

        for(noz_uint32 p=0; p<VisualStudioPlatformCount;p++) {
          writer.Write("      <PrecompiledHeader Condition=\"'$(Configuration)|$(Platform)'=='");
          writer.Write(cfg.name_);
          writer.Write("|");
          writer.Write(VisualStudioPlatforms[p]);
          writer.Write("'\">");
          if(file.precompiled_header_==Makefile::PrecompiledHeader::Create) {
            writer.Write("Create");
          } else {
            writer.Write("NotUsing");
          }
          writer.Write("</PrecompiledHeader>\r\n");
        }
      }
      writer.Write("    </ClCompile>\r\n");
    } else {
      writer.Write(" />\r\n");
    }
  }
  writer.Write("  </ItemGroup>\r\n");

  // <ItemGroup> <ClInclude>
  writer.Write("  <ItemGroup>\r\n");
  for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
    const Makefile::BuildFile& file = *makefile->files_[i];
    if(GetItemType(file) != ItemType::ClInclude) continue;

    writer.Write("    <ClInclude Include=\"");
    writer.Write(file.path_);
    writer.Write("\" />\r\n");
  }
  writer.Write("  </ItemGroup>\r\n");

  // <ItemGroup> <ClInclude>
  writer.Write("  <ItemGroup>\r\n");
  for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
    const Makefile::BuildFile& file = *makefile->files_[i];
    if(GetItemType(file) != ItemType::None) continue;

    writer.Write("    <None Include=\"");
    writer.Write(file.path_);
    writer.Write("\" />\r\n");
  }
  writer.Write("  </ItemGroup>\r\n");

  // REFERENCES
  if(!makefile->references_.empty()) {
    writer.Write("  <ItemGroup>\r\n");
    for(noz_uint32 i=0,c=makefile->references_.size(); i<c; i++) {
      Makefile* ref = GetReferenceMakefile(makefile, makefile->references_[i].path_);
      if(nullptr == ref) continue;
        
      String relative_project_path = Path::GetRelativePath(ref->target_path_,target_dir);
      relative_project_path = Path::GetWindowsPath(relative_project_path);


      writer.Write("    <ProjectReference Include=\"");
      writer.Write(relative_project_path);
      writer.Write("\">\r\n");
      writer.Write("      <Project>{");
      writer.Write(ref->guid_.ToString());
      writer.Write("}</Project>\r\n");
      writer.Write("    </ProjectReference>\r\n");
    }
    writer.Write("  </ItemGroup>\r\n");
  }

  writer.Write("  <ItemGroup>\r\n");
  for(noz_uint32 i=0,c=makefile->files_.size(); i<c; i++) {
    const Makefile::BuildFile& file = *makefile->files_[i];
    if(GetItemType(file) != ItemType::ResourceCompile) continue;

    writer.Write("    <ResourceCompile Include=\"");
    writer.Write(file.path_);
    writer.Write("\" />\r\n");
  }
  writer.Write("  </ItemGroup>\r\n");

  writer.Write("  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\r\n");
  writer.Write("</Project>\r\n");
      
  if(!WriteFilters(makefile, target_dir)) {
    return false;
  }

  if(!WriteSolution(makefile, target_dir)) {
    return false;
  }
  return true;
}

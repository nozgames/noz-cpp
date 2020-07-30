///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef _noz_XCodeProjectGen_h__
#define _noz_XCodeProjectGen_h__

#include <noz/Editor/Tool/ProjectGen/ProjectGen.h>

namespace noz {

  class XCodeProjectGen : public ProjectGen {
    private: enum class BuildPhase {
      Unknown,
      Resources,
      Sources,
      CopyFiles,
      Frameworks
    };

    private: class FileReference {
      public: String guid_;
      public: String last_known_file_type_;
      public: String explicit_file_type_;
      public: String name_;
      public: String source_tree_;
      public: String path_;
      public: String group_;
      public: noz_int32 include_in_index_;
      public: noz_int32 file_encoding_;

      public: FileReference(void) : include_in_index_(-1), file_encoding_(-1) {}
    };

    private: class BuildFile {
      public: String name_;
      public: String guid_;
      public: String file_reference_guid_;
      public: BuildPhase phase_;
      public: FileReference* file_reference_;

      public: BuildFile (void) : file_reference_(nullptr), phase_(BuildPhase::Unknown) {}
    };

    private: class CopyFilesBuildPhase {
      public: String guid_;
      public: String file_guid_;
      public: String dst_path_;
    };

    private: class Reference {
      public: String pbx_container_item_proxy_1_guid_;
      public: String pbx_container_item_proxy_2_guid_;
      public: String pbx_target_dependency_guid_;
      public: String pbx_reference_proxy_guid_;
      public: String pbx_group_id_;
      public: String container_portal_guid_;
      public: BuildFile* build_file_;
      public: FileReference* file_ref_;
      public: FileReference product_file_ref_;
      public: Makefile* makefile_;
    };

    private: class Group {
      public: String name_;
      public: String guid_;
      public: std::vector<Group*> groups_;
      public: std::vector<FileReference*> file_references_;
    };

    private: class Project {
      public: Makefile* makefile_;
      public: noz_int32 next_guid_;
      public: String target_path_;
      public: String target_filename_;
      public: String output_filename_;
      public: String plist_path_;
      public: String plist_filename_;

      public: String pbx_project_guid_;
      public: String main_pbx_group_guid_;
      public: String products_pbx_group_guid_;
      public: String reference_products_pbx_group_guid_;
      public: String pbx_native_target_xconfiguration_list_guid_;
      public: String pbx_frameworks_build_phase_guid_;
      public: String pbx_sources_build_phase_guid_;
      public: String pbx_resources_build_phase_guid_;
      public: String pbx_native_target_guid_;
      public: String product_pbx_file_reference_guid_;
      public: String pbx_project_xconfiguration_list_guid_;
      public: String glue_pbx_shell_script_build_phase_guid_;

      public: Group* main_group_;

      public: std::vector<BuildFile*> build_files_;
      public: std::vector<FileReference*> file_references_;
      public: std::vector<CopyFilesBuildPhase*> copy_file_build_phases_;
      public: std::vector<Reference*> references_;
      public: std::vector<Group*> groups_;
      public: std::map<Name,Group*> groups_by_name_;
      public: bool has_resources_;

      public: String GenerateGUID (void) {
        noz_int32 guid = next_guid_++;
        return String::Format("FF000000FF000000%08X", guid);
      }

      public: Project(void) : has_resources_(false) { }
    };

    public: virtual bool GenerateOverride (Makefile* makefile, const String& target_dir) override;

    private: String BuildPhaseToString (BuildPhase p) const;

    private: void AddFramework (Project& project, const String& name, const String& path);
    private: Group* AddGroup (Project& project, const String& name);
    private: bool WritePLIST (Project& project);

    private: void WritePBXValue (TextWriter& writer, const String& value);
    private: void WritePBXProperty (TextWriter& writer, noz_int32 indent, const String& name, const String& value, const String& comment=String::Empty);
    private: void WritePBXProperty (TextWriter& writer, const String& name, const String& value, const String& comment=String::Empty);
    private: void WritePBXListPropertyPrologue(TextWriter& writer, noz_int32 indent, const String& name);
    private: void WritePBXListProperty(TextWriter& writer, noz_int32 indent, const String& value, const String& comment=String::Empty);
    private: void WritePBXListPropertyEpilogue(TextWriter& writer, noz_int32 indent);
    private: void WritePBXElementPrologue (TextWriter& writer, noz_int32 indent, bool single_line, const String& guid, const String& comment=String::Empty);
    private: void WritePBXElementEpilogue (TextWriter& writer, noz_int32 indent);
    private: void WritePBXElementEpilogue (TextWriter& writer);

    private: bool WriteBuildConfigurationList (Project& project, TextWriter& writer, const char* comment, const String& guid_buildcfglist, bool native);

    private: bool WritePBXBuildFileSection (Project& project, TextWriter& writer);
    private: bool WritePBXContainerItemProxySection (Project& project, TextWriter& writer);
    private: bool WritePBXCopyFilesBuildPhaseSection (Project& project, TextWriter& writer);
    private: bool WritePBXFileReferenceSection (Project& project, TextWriter& writer);
    private: bool WritePBXFrameworksBuildPhaseSection (Project& project, TextWriter& writer);
    private: bool WritePBXGroupSection (Project& project, TextWriter& writer);

    private: bool WritePBXNativeTargetSection (Project& project, TextWriter& writer);
    private: bool WritePBXProjectSection (Project& project, TextWriter& writer);
    private: bool WritePBXReferenceProxySection (Project& project, TextWriter& writer);
    private: bool WritePBXResourcesBuildPhaseSection (Project& project, TextWriter& writer);
    private: bool WritePBXShellScriptBuildPhaseSection (Project& project, TextWriter& writer);
    private: bool WritePBXSourcesBuildPhaseSection (Project& project, TextWriter& writer);
    private: bool WritePBXTargetDependencySection (Project& project, TextWriter& writer);
    private: bool WriteXCBuildConfigurationSection (Project& project, TextWriter& writer);
    private: bool WriteXCConfigurationListSection (Project& project, TextWriter& writer);  
  };

}

#endif // _noz_XCodeProjectGen_h__

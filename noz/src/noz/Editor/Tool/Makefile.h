///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Makefile_h__
#define __noz_Makefile_h__

namespace noz {

  class JsonReader;

  class Makefile {
    public: enum class PlatformType {
      Unknown,
      Windows,
      IOS,
      OSX
    };

    public: enum class TargetType {
      Application,
      Library,
      Console
    };

    public: enum class PrecompiledHeader {
      None,
      Use,
      Create
    };

    public: struct ParseOptions {
      /// Platform to build.
      PlatformType platform;

      /// Additional preprocessor definitions
      std::set<Name> defines;

      /// Directory to play output.  
      String output_directory_;

      ParseOptions (void) : platform (PlatformType::Windows) { }
    };

    public: class Configuration {
      public: String name_;
      public: std::vector<String> libraries_;
      public: std::vector<String> include_directories_;
      public: PrecompiledHeader precompiled_header_;
      public: String precompiled_header_file_;
      public: std::vector<String> preprocessor_definitions_;
      public: bool optimizations_;
      public: bool warnings_;
      public: String bin_directory_;
      public: String output_directory_;

      public: String xcode_project_guid_;
      public: String xcode_native_target_guid_;

      public: struct {
        noz_uint32 precompiled_header_is_default_: 1;
        noz_uint32 precompiled_header_file_is_default_: 1;
        noz_uint32 optimizations_is_default_: 1;
        noz_uint32 warnings_is_default_: 1;
      };

      public: Configuration(void) : 
        precompiled_header_is_default_(true),
        precompiled_header_file_is_default_(true),
        optimizations_is_default_(true),
        warnings_is_default_(true),
        precompiled_header_(PrecompiledHeader::Use),
        warnings_(true),
        optimizations_(false) { }
    };

    public: class BuildFile {
      public: String path_;
      public: String group_;
      public: bool directory_;
      public: bool glue_;
      public: PrecompiledHeader precompiled_header_;
      public: BuildFile (void) : glue_(true), precompiled_header_(PrecompiledHeader::Use), directory_(false) { }
    };

    public: class Reference {
      public: String path_;
    };

    /// Path of the makefile itself
    public: String path_;
    public: String name_;
    public: ParseOptions options_;

    public: String organization_;
    public: String bundle_id_;
    public: Guid guid_;
    public: Guid solution_guid_;
    public: PlatformType platform_;
    public: String target_;
    public: TargetType target_type_;
    public: String target_path_;
    public: String target_dir_;
    public: String auto_group_directory_;
    public: String auto_group_base_;
    public: bool glue_;
    public: std::vector<String> assets_directories_;
    public: std::vector<BuildFile*> files_;
    public: std::map<Name,BuildFile*> files_by_name_;
    public: std::vector<Configuration*> configurations_;
    public: std::vector<Reference> references_;
    public: Configuration default_configuration_;

    public: String xcode_product_guid_;
    public: String xcode_native_target_guid_;
    public: String xcode_output_filename_;

    public: Makefile (void) : target_type_(TargetType::Application), glue_(false), platform_(PlatformType::Windows) { }

    public: static Makefile* Parse (const String& path, const ParseOptions& options);

    public: static String Preprocess (const char* text, const ParseOptions& options);

    private: static bool Parse (Makefile* makefile, JsonReader& reader);
    private: static bool ParseFiles (Makefile* makefile, JsonReader& reader);
    private: static bool ParseConfigurations (Makefile* makefile, JsonReader& reader);
    private: static bool ParseConfiguration (Makefile* makefile, Configuration* configuration, JsonReader& reader);

    private: static bool MergeConfiguration (Configuration* src, Configuration* dst);

    private: BuildFile* GetBuildFile (const String& path);
    private: void RemoveBuildFile (const String& path);

    public: static const String& PlatformTypeToString(PlatformType pt);
    public: static PlatformType StringToPlatformType (const char* s);
  };

} // namespace noz

#endif // __noz_Makefile_h__

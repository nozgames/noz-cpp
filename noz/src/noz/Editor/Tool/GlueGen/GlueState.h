///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueState_h__
#define __noz_Editor_GlueState_h__

namespace noz {
namespace Editor {

  struct GlueProperty;
  struct GlueEnum;
  struct GlueFile;
  struct GlueMethod;
  struct GlueType;
  struct GlueClass;
}
}

#include "GlueLexer.h"
#include "GlueMeta.h"
#include "GlueType.h"
#include "GlueFile.h"
#include "GlueEnum.h"
#include "GlueProperty.h"
#include "GlueParameter.h"
#include "GlueMethod.h"
#include "GlueClass.h"

namespace noz {
namespace Editor {

  struct GlueState {
    friend class GlueGen;

    /// Map of all classes by their unqualified name
    private: std::map<Name,GlueClass*> classes_by_name_;

    /// Map of all classes by their qualified name
    private: std::map<Name,GlueClass*> classes_by_qualified_name_;

    /// Vector of all known classes
    private: GlueClassVector classes_;

    /// Map of all enums by qualified name
    private: std::map<Name,GlueEnum*> enums_by_qualified_name_;

    /// Vector of all known enums
    private: GlueEnumVector enums_;

    /// Map of all types by their qualified names
    private: std::map<Name,GlueType> types_by_qualified_name_;

    /// Include directories to search when searching for a header file.
    private: std::set<Name> include_directories_;

    /// Map of all glue files by their full canonical path.
    private: std::map<Name,GlueFile*> files_by_full_path_;

    /// Vector of all known files.
    private: GlueFileVector files_;

    /// Defines use for loading the make file.
    private: std::set<Name> defines_;

    /// Platform to compile glue for
    private: Makefile::PlatformType platform_;
    
    /// True if the -clean option was specified on command line
    private: bool clean_;

    /// Full path to the glue tool executable file
    private: String glue_exe_path_;

    /// Current working directory when application was starteed
    private: String current_path_;

    /// Full path to the makefile
    private: String makefile_path_;

    /// Full canonical directory the project file is contained in.
    private: String makefile_directory_;

    /// Path to the output glue file (.glue)
    private: String output_path_;

    private: String target_directory_;

    /// Name of the configuration being processed
    private: String config_name_;

    /// Path to the PCH file.
    private: GlueFile* precompiled_header_;


    public: GlueState (void);

    public: GlueClass* AddClass(const Name& ns, const Name& name);

    public: GlueClass* AddClass(GlueClass* gc);

    public: GlueEnum* AddEnum(const Name& ns, const Name& name);

    public: const GlueFileVector& GetFiles(void) const {return files_;}

    /// Return true if a build is required.
    public: bool IsBuildRequired (void) const;

    /// Add a new glue file to the state.  
    public: bool AddFile(GlueFile* gf);

    /// Find a file by its full canonical path
    public: GlueFile* GetFile (const Name& full_path) const;

    /// Return the vector of classes.
    public: const GlueClassVector& GetClasses (void) const {return classes_;}

    /// Return the class matching the given unqualified name
    public: GlueClass* GetClass (const Name& name) const;

    /// Return the class matching the given qualified name
    public: GlueClass* GetQualifiedClass (const Name& qualified_name) const;

    /// Return the enum matching the given qualified name
    public: GlueEnum* GetQualifiedEnum (const Name& qualified_name) const;

    /// Return the vector of enums.
    public: const GlueEnumVector& GetEnums (void) const {return enums_;}

    /// Return the type that matches the given qualified name
    public: GlueType* GetQualifiedType (const Name& qualified_name);

    /// Return the output path.
    public: const String& GetOutputPath(void) const {return output_path_;}
   
    /// Process all classes
    public: bool ProcessClasses (void);

    /// Add an include directory for the entire project
    public: void AddIncludeDirectory (const Name& dir);

    /// Return the additional include directories for the entire project
    public: const std::set<Name>& GetIncludeDirectories(void) const {return include_directories_;}

    /// Load the project specified on the command line
    public: bool LoadProject (void);

    /// Resolve the class name using the given namespace
    public: GlueClass* ResolveClass (const Name& name, const Name& _ns) const;

    /// Resolve the enum name using the given namespace
    public: GlueEnum* ResolveEnum (const Name& name, const Name& _ns) const;

    public: GlueFile* GetPrecompiledHeader (void) const {return precompiled_header_;}

    public: const String& GetProjectPath (void) const {return makefile_path_;}

    public: const String& GetConfigName (void) const {return config_name_;}

    private: static int GlueClassSort(const void* _gc1, const void* _gc2);
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GlueState_h__

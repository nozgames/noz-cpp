///////////////////////////////////////////////////////////////////////////////
// noZ Glue Compiler
// Copyright (C) 2013-2014 Bryan Dube / Radius Software
// http://www.radius-software.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueFile_h__
#define __noz_Editor_GlueFile_h__

#include "GlueLexer.h"

namespace noz {
namespace Editor {

  enum class GlueFileType {
    Unknown,
    CPP,
    C,
    H,
    PCH
  };

  struct GlueFile {
    /// Type of file.
    private: GlueFileType type_;

    /// Full canonical OS path.
    private: String full_path_;

    /// File path as found in the project
    private: String path_;

    /// Directory the file resides 
    private: String directory_;
    
    /// If true the file and any classes defined in it should be 
    /// excluded from final output
    private: bool excluded_;

    private: bool glue_;

    /// Include directories to search when searching for a header file.
    private: std::set<Name> include_directories_;
    
    /// Raw file data.
    private: std::vector<char> data_;
  
    /// Default constructor
    public: GlueFile (void);

    public: void SetFullPath (const String& path);

    public: void SetPath (const String& path);

    public: void SetExcluded (bool excluded);

    public: void SetGlueFile (bool glue) {glue_ = glue;}

    public: const String& GetPath (void) const {return path_;}

    public: const String& GetFullPath (void) const {return full_path_;}

    public: const String& GetDirectory (void) const {return directory_;}

    public: GlueFileType GetType (void) const {return type_;}

    public: bool IsExcluded (void) const {return excluded_;}

    public: bool IsGlueFile (void) const {return glue_;}

    public: bool IsHeader (void) const {return type_==GlueFileType::H || type_==GlueFileType::PCH;}

    public: bool IsPrecompiledHeader (void) const {return type_==GlueFileType::PCH;}

    public: GlueLexer* CreateLexer (void);

    /// Add an include directory for the file
    public: void AddIncludeDirectory (const Name& dir) {include_directories_.insert(dir);}

    /// Return the additional include directories for the file
    public: const std::set<Name>& GetIncludeDirectories(void) const {return include_directories_;}

    /// Load the contents of the file at the file path into the internal lexer
    public: bool Load (void);

    public: const char* GetData (void) const {return &data_[0];}

    public: static GlueFileType GetGlueFileType (const String& path);

    private: void Preprocess (void);
  };

  typedef std::vector<GlueFile*> GlueFileVector;

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GlueFile_h__

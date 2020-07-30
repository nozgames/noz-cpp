///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueEnum_h__
#define __noz_Editor_GlueEnum_h__

namespace noz {
namespace Editor {

  struct GlueEnum {
    /// True if the enum is excluded from the build
    private: bool excluded_;

    /// True if the enum is a reflected enum
    private: bool reflected_;

    /// Fully qualified name of the enumeration
    private: Name qualified_name_;

    /// Unqualified name
    private: Name name_;

    /// Namespace the enum belongs to
    private: Name namespace_;

    private: GlueFile* gf_;

    private: noz_int32 gf_line_;

    private: GlueMeta meta_;
    
    private: std::multimap<noz_int32,Name> name_by_value_;

    private: std::map<Name,noz_int32> value_by_name_;



    public: GlueEnum(void) {
      excluded_ = true;
      reflected_ = false;
      gf_ = nullptr;
      gf_line_ = 0;
    }

    public: const Name& GetQualifiedName(void) const {return qualified_name_;}
    public: GlueFile* GetFile (void) const {return gf_;}
    public: noz_uint32 GetLineNumber (void) const {return gf_line_;}

    public: bool IsExcluded (void) const {return excluded_;}
    public: bool IsReflected (void) const {return reflected_;}

    public: void SetReflected (bool reflected) {reflected_ = reflected;}
    public: void SetExcluded (bool excluded) {excluded_ = excluded;}
    public: void SetQualifiedName (const Name& name) {qualified_name_ = name;}
    public: void SetName (const Name& _name) {name_ = _name;}
    public: void SetNamespace (const Name& ns) {namespace_ = ns;}
    public: void SetFile (GlueFile* gf, noz_uint32 line=0) {gf_=gf;gf_line_=line;}

    public: const std::multimap<noz_int32,Name>& GetValues (void) const {return name_by_value_;}
    public: const std::map<Name,noz_int32>& GetNames (void) const {return value_by_name_;}

    public: bool Contains (const Name& name) const {return value_by_name_.find(name)!=value_by_name_.end();}
    public: noz_int32 GetValue(const Name& name) const {auto it=value_by_name_.find(name); if(it!=value_by_name_.end()) return it->second; return 0;}

    public: void AddValue (const Name& name, noz_int32 value) {
      value_by_name_[name] = value;
      name_by_value_.insert(std::pair<int,Name>(value,name));      
    }
  };


  typedef std::vector<GlueEnum*> GlueEnumVector;

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GlueEnum_h__

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueMeta_h__
#define __noz_Editor_GlueMeta_h__

namespace noz {
namespace Editor {

  struct GlueMeta {
    public: static const Name NameAbstract;
    public: static const Name NameTypeCode;
    public: static const Name NameManaged;
    public: static const Name NameReadOnly;
    public: static const Name NameWriteOnly;
    public: static const Name NameType;
    public: static const Name NameName;
    public: static const Name NameSet;
    public: static const Name NameGet;
    public: static const Name NameSetSize;
    public: static const Name NameGetSize;
    public: static const Name NameSetElement;
    public: static const Name NameAdd;
    public: static const Name NameElement;    
    public: static const Name NameNonSerialized;

    private: static std::set<Name> reserved_names_;

    private: std::map<Name,String> values_by_name_;

    public: GlueMeta (void) {
    }

    public: void Set(const Name& name, const String& value) {
      values_by_name_[name] = value;
    }

    public: void Set(const Name& name) {
      values_by_name_[name] = "";
    }

    public: bool GetBool (const Name& name, bool def=false) const {
      auto it=values_by_name_.find(name);
      if(it==values_by_name_.end()) return def; 
      return Boolean::Parse(it->second);
    }

    public: bool GetAndRemoveBool (const Name& name, bool def=false) {
      auto it=values_by_name_.find(name);
      if(it==values_by_name_.end()) return def;
      String result = it->second;
      values_by_name_.erase(it);
      if(result.IsEmpty()) return true;
      return Boolean::Parse(result);
    }

    public: const String& GetString (const Name& name, const String& def=String::Empty) const {
      auto it=values_by_name_.find(name);
      if(it==values_by_name_.end()) return def;
      return it->second;
    }

    public: String GetAndRemoveString (const Name& name, const String& def=String::Empty)  {
      auto it=values_by_name_.find(name);
      if(it==values_by_name_.end()) return def;
      String result = it->second;
      values_by_name_.erase(it);
      return result;
    }

    public: bool Contains (const Name& name) const {
      return values_by_name_.find(name) != values_by_name_.end();
    }

    public: const std::map<Name,String>& GetValues(void) const {return values_by_name_;}


    public: static void InitializeReservedNames(void);

    public: static bool IsReservedName(const Name& name) {return reserved_names_.find(name) != reserved_names_.end();}
  };


} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GlueMeta_h__

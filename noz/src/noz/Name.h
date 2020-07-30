///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Name_h__
#define __noz_Name_h__

namespace noz {

  class NameHash;
  class NameValue;

  class Name {
    protected: NameValue* value_;

    private: static NameHash* hash_;

    public: static Name Empty;

    public: Name(void) : Name(Name::Empty) {}

    public: Name(const char* value);

    public: Name(const String& value);

    public: Name(const Name& value);

    public: ~Name (void);

    public: const char* ToCString(void) const;

    public: const String& ToString(void) const;

    public: operator const String& (void) const {return ToString();}

    public: bool IsEmpty(void) const {return *this==Empty;}

    public: Name& operator= (const Name& v);

    public: Name& operator= (const String& v);

    public: Name& operator= (const char* v);

    public: bool operator== (const Name& o) const {return value_ == o.value_;}
    public: bool operator!= (const Name& o) const {return value_ != o.value_;}
    
    public: bool operator< (const Name& o) const;
    public: bool operator> (const Name& o) const;

    /// Purge all unreferenced names in the hash
    public: static void Purge (void);

    /// Set the name system to purge all names when their reference counts hits zero. This 
    /// is most useful for termination of the application as it ensures that global names
    /// are cleaned up as they are destructed/
    public: static void SetPurgeOnUnreferenced(bool purge_unreferenced);
  };

} // namespace noz


#endif //__noz_Name_h__



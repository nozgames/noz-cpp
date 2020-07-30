///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_PropertyPath_h__
#define __noz_PropertyPath_h__

namespace noz {

  class PropertyPathProperty;

  /** 
   * Describes a path to a property 
   */
  class PropertyPath {
    friend class PropertyPathProperty;

    protected: std::vector<Name> names_;

    public: PropertyPath(void);

    public: PropertyPath(const char* path) {*this = path;}

    public: PropertyPath(const String& path) : PropertyPath(path.ToCString()) {}

    public: PropertyPath(const Name& name);

    public: PropertyPath& operator= (const char* path);

    public: PropertyPath& operator= (const String& path) {*this = path.ToCString(); return *this; }

    public: PropertyPath& operator= (const Name& path);

    public: noz_uint32 GetLength(void) const {return names_.size();}

    public: const Name& operator[] (noz_uint32 index) const {return names_[index];}
  };

}

#endif // __noz_PropertyPath_h__

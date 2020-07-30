///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_NodePath_h__
#define __noz_NodePath_h__

namespace noz {

  class NodePathProperty;

  /** 
   * Describes a path to a node using an array of discreet names
   */
  class NodePath {
    friend class NodePathProperty;

    protected: std::vector<Name> names_;

    protected: bool relative_;

    public: NodePath(void);

    public: NodePath(const char* path) {*this = path;}

    public: NodePath(const String& path) : NodePath(path.ToCString()) {}

    public: NodePath(const Name& name);

    public: bool IsRelative(void) const {return relative_;}

    public: bool IsEmpty(void) const {return names_.empty() || (names_.size()==1 && names_[0].IsEmpty());}

    public: NodePath& operator= (const char* path);

    public: NodePath& operator= (const String& path) {*this = path.ToCString(); return *this; }

    public: NodePath& operator= (const Name& path);

    public: noz_uint32 GetLength(void) const {return names_.size();}

    public: const Name& operator[] (noz_uint32 index) const {return names_[index];}
  };


}

#endif // __noz_NodePath_h__

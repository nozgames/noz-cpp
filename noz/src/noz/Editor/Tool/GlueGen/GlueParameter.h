///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueParameter_h__
#define __noz_Editor_GlueParameter_h__

namespace noz {
namespace Editor {

  struct GlueParameter {
    private: GlueType* gt_;
    private: Name name_;
    private: bool default_;

    public: GlueParameter(void) {
      gt_ = nullptr;
      default_ = false;
    }

    public: const Name& GetName(void) const {return name_;}
    public: GlueType* GetType(void) const {return gt_;}

    public: void SetName(const Name& n) {name_ = n;}
    public: void SetDefault (bool v) {default_ = v;}
    public: void SetType(GlueType* gt) {gt_ = gt;}

    public: bool IsDefault (void) const {return default_;}
  };

  typedef std::vector<GlueParameter*> GlueParameterVector;

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GlueParameter_h__

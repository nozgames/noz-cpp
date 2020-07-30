///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueProperty_h__
#define __noz_Editor_GlueProperty_h__

namespace noz {
namespace Editor {

  struct GlueProperty {
    private: Name name_;

    private: GlueFile* gf_;

    private: noz_int32 gf_line_;

    private: GlueMeta meta_;

    private: GlueType* gt_;

    private: int array_rank_;




    public: GlueProperty(void) { 
      array_rank_ = 0; 
      gt_ = nullptr;
      gf_ = nullptr;
      gf_line_ = 0;
    }

    public: void SetName(const Name& name) {name_ = name;}

    public: void SetArrayRank (noz_int32 ar) {array_rank_ = ar;}

    public: void SetType (GlueType* gt) {gt_ = gt;}

    public: const Name& GetName(void) const {return name_;}

    public: GlueType* GetType(void) const {return gt_;}

    public: GlueMeta& GetMeta(void) {return meta_;}

    /// Return the file the property was found in
    public: GlueFile* GetFile (void) const {return gf_;}

    /// Return the line number the property was found on
    public: noz_uint32 GetLineNumber (void) const {return gf_line_;}

    /// Set the file and line number
    public: void SetFile (GlueFile* gf, noz_uint32 line=0) {gf_=gf;gf_line_=line;}

    public: bool IsArray(void) const {return array_rank_>0;}

    public: noz_int32 GetArrayRank(void) const {return array_rank_;}
  };

  typedef std::vector<GlueProperty*> GluePropertyVector;

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GlueProperty_h__



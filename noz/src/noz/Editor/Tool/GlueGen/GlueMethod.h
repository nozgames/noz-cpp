///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueMethod_h__
#define __noz_Editor_GlueMethod_h__

namespace noz {
namespace Editor {

  struct GlueMethod {
    private: Name name_;
    private: bool reflected_;
    private: bool virtual_;
    private: bool abstract_;
    private: bool constructor_;
    private: bool static_;
    private: bool const_;
    private: GlueLexer::TokenType access_;
    private: GlueMeta meta_;
    private: GlueClass* gc_;
    private: GlueType* return_type_;
    private: GlueParameterVector params_;

    public: GlueMethod(void) {
      return_type_ = nullptr;
      abstract_ = false;
      virtual_ = false;
      constructor_ = false;
      static_ = false;
      const_ = false;
      reflected_ = false;
      access_ = GlueLexer::TokenType::Private;
    }

    public: void SetName(const Name& n) {name_ = n;}
    public: void SetAbstract (bool a) {abstract_ = a;}
    public: void SetVirtual (bool v) {virtual_ = v;}
    public: void SetConstructor (bool c) {constructor_ = c;}
    public: void SetStatic (bool v) {static_ = v;}
    public: void SetConst (bool c) {const_ = c;}
    public: void SetClass (GlueClass* gc) {gc_ = gc;}
    public: void SetAccess (GlueLexer::TokenType access) {access_ = access;}
    public: void SetReturnType (GlueType* gt) {return_type_ = gt;}
    public: void SetReflected (bool v) {reflected_ = v;}

    public: void AddParameter(GlueParameter* gparam) {params_.push_back(gparam);}

    public: bool IsAbstract (void) const {return abstract_;}
    public: bool IsVirtual (void) const {return virtual_;}
    public: bool IsConstructor (void) const {return constructor_;}
    public: bool IsStatic (void) const {return static_;}
    public: bool IsConst (void) const {return const_;}
    public: bool IsReflected (void) const {return reflected_;}

    public: GlueMeta& GetMeta(void) {return meta_;}
    public: const Name& GetName(void) const {return name_;}
    public: GlueClass* GetClass(void) const {return gc_;}
    public: GlueLexer::TokenType GetAccess(void) const {return access_;}
    public: GlueType* GetReturnType(void) const {return return_type_;}
    public: const GlueParameterVector& GetParameters(void) const {return params_;}
  };

  typedef std::vector<GlueMethod*> GlueMethodVector;

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GlueMethod_h__

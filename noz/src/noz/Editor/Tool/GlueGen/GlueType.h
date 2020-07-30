///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueType_h__
#define __noz_Editor_GlueType_h__

namespace noz {
namespace Editor {

  struct GlueType {
    private: String raw_;
    private: Name namespace_;
    private: GlueType* template_type_;
    private: GlueType* template_arg_type_;
    private: GlueClass* gc_;
    private: GlueEnum* ge_;
    private: bool pointer_;
    private: bool reference_;
    private: bool const_;

    public: GlueType(void) {
      template_type_ = nullptr;
      template_arg_type_ = nullptr;
      gc_ = nullptr;
      ge_ = nullptr;
      pointer_ = false;
      const_ = false;
      reference_ = false;
    }

    public: const String& GetRaw(void) const {return raw_;}
    public: const Name& GetNamespace(void) const {return namespace_;}
    public: GlueType* GetTemplateType(void) const {return template_type_;}
    public: GlueType* GetTemplateArgType(void) const {return template_arg_type_;}
    public: GlueClass* GetClass(void) const {return gc_;}
    public: GlueEnum* GetEnum(void) const {return ge_;}

    public: bool IsTemplate(void) const {return template_type_ != nullptr;}
    public: bool IsTemplate(GlueClass* gc) const {return template_type_ && template_type_->IsClass(gc);}
    public: bool IsPointer(void) const {return pointer_;}
    public: bool IsReference(void) const {return reference_;}
    public: bool IsEnum(void) const {return ge_!=nullptr;}
    public: bool IsClass(void) const {return gc_!=nullptr;}
    public: bool IsClass(GlueClass* gc) const {return gc_ == gc;}
    public: bool IsClassPointer(void) const {return gc_ && IsPointer();}
    public: bool IsClassPointer(GlueClass* gc) const {return gc_ == gc && IsPointer();}
    public: bool IsCastableTo(GlueClass* gc) const;

    public: void SetRaw(const String& raw) {raw_ = raw;}
    public: void SetNamespace (const Name& ns) {namespace_ = ns;}
    public: void SetClass(GlueClass* gc) {gc_ = gc;}
    public: void SetEnum(GlueEnum* ge) {ge_ = ge;}
    public: void SetPointer(bool v) {pointer_ = v;}
    public: void SetReference(bool v) {reference_ = v;}
    public: void SetConst(bool v) {const_ = v;}
    public: void SetTemplate(GlueType* tt, GlueType* tat) {template_type_=tt; template_arg_type_=tat;}
  };

  typedef std::vector<GlueType*> GlueTypeVector;

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_GlueType_h__

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "GlueGen.h"

namespace noz {
namespace Editor {
namespace Names {
  static const Name Add("Add");
  static const Name Clear("Clear");
  static const Name Get("Get");
  static const Name GetCount("GetCount");
  static const Name Insert("Insert");
  static const Name Remove("Remove");
  static const Name Release("Release");
  static const Name Reserve("Reserve");
  static const Name Private("Private");
  static const Name ControlPart("ControlPart");
  static const Name DefaultTemplate("DefaultTemplate");
}
}
}

using namespace noz;
using namespace noz::Editor;

bool GlueGen::WriteEnums(GlueState* gs, StringBuilder& out) {
  for(auto it=gs->GetEnums().begin(); it!=gs->GetEnums().end(); it++) {
    GlueEnum* ge = *it;
    noz_assert(ge);

    if(ge->IsExcluded()) continue;
    if(!ge->IsReflected()) continue;

    out.Append("  // enum class ");
    out.Append(ge->GetQualifiedName());
    out.Append("\n");

    out.Append("  {Type* t = Enum<");
    out.Append(ge->GetQualifiedName());
    out.Append(">::type__ = new Type(\"");
    out.Append(ge->GetQualifiedName());
    out.Append("\",0,TypeCode::Enum,nullptr); Enum<");
    out.Append(ge->GetQualifiedName());
    out.Append(">::GetName = [] (");
    out.Append(ge->GetQualifiedName());
    out.Append(" value) -> const Name& {switch(value){");
  
    for(auto it=ge->GetValues().begin(); it!=ge->GetValues().end(); it++) {
      out.Append("case ");
      out.Append(ge->GetQualifiedName());
      out.Append("::");
      out.Append(it->second);
      out.Append(": {static Name n(\"");
      out.Append(it->second);
      out.Append("\"); return n;} ");
    }

    out.Append("} return Name::Empty;}; Enum<");
    out.Append(ge->GetQualifiedName());
    out.Append(">::GetValue = [] (const Name& value) -> ");
    out.Append(ge->GetQualifiedName());
    out.Append(" {static std::map<Name,");
    out.Append(ge->GetQualifiedName());
    out.Append("> dict;if(dict.empty()) {");
    
    for(auto it=ge->GetNames().begin(); it!=ge->GetNames().end(); it++) {
      out.Append("dict[Name(\"");
      out.Append(it->first);
      out.Append("\")]=");
      out.Append(ge->GetQualifiedName());
      out.Append("::");
      out.Append(it->first);
      out.Append(";");
    }

    out.Append("} return dict[value];};");

    out.Append(" Enum<");
    out.Append(ge->GetQualifiedName());
    out.Append(">::GetNames = [] (void) -> const std::vector<Name>& ");
    out.Append(" {static std::vector<Name> names;");
    out.Append("if(names.empty()) {names.reserve(");
    out.Append(String::Format("%d",ge->GetNames().size()));
    out.Append("); ");

    for(auto it=ge->GetNames().begin(); it!=ge->GetNames().end(); it++) {      
      out.Append("names.push_back(\"");
      out.Append(it->first);
      out.Append("\"); ");
    }

    out.Append("} return names;}; ");

    out.Append("Type::RegisterType(t); }\n");
  }

  return true;
}

bool GlueGen::WriteMethods(GlueState* gs, GlueClass* gc, StringBuilder& out) {
  for(auto it=gc->GetMethods().begin(); it!=gc->GetMethods().end(); it++) {
    GlueMethod* gm = *it;
    noz_assert(gm);

    // Do not export static methods.
    if(gm->IsStatic()) continue;

    // Skip methods that are not marked for reflection with NOZ_METHOD
    if(!gm->IsReflected()) continue;

    out.Append("  {class M : public Method { "); 
    out.Append("public: M(void) : Method(0");
   
    out.Append("){} public: virtual Type* GetTargetType(void) const {return typeof(");
    out.Append(gc->GetQualifiedName());
    out.Append(");} public: virtual void Invoke (Object* t, noz_int32 argc, Object* argv[]) {");

    // Validate the number of parameters
    out.Append("if(argc!=");
    out.Append(UInt32(gm->GetParameters().size()).ToString());
    out.Append(") return;");

    // Validate each parameter
    int i=0;
    for(auto itp=gm->GetParameters().begin(); itp != gm->GetParameters().end(); itp++) {
      GlueParameter* gp = *itp;
      out.Append("if(argv[");
      out.Append(Int32(i).ToString());
      out.Append("] && !argv[");
      out.Append(Int32(i).ToString());
      out.Append("]->IsTypeOf(typeof(");
      out.Append(gp->GetType()->GetClass()->GetQualifiedName());
      out.Append("))) return;");
    }

    if(gm->GetReturnType()!=nullptr) {
      out.Append("((");
      out.Append(gm->GetReturnType()->GetRaw());
      out.Append(")");
    }

    out.Append("((");
    out.Append(gc->GetQualifiedName());
    out.Append("*)t)->");
    out.Append(gm->GetName());
    out.Append("(");

    for(size_t pi=0; pi<gm->GetParameters().size(); pi++) {
      GlueParameter* gp = gm->GetParameters()[pi];
      noz_assert(gp);

      if(pi!=0) out.Append(",");
      out.Append("((");
      out.Append(gp->GetType()->GetClass()->GetQualifiedName());
      out.Append("*)argv[");
      out.Append(UInt32(pi).ToString());
      out.Append("])");
    }
    
    out.Append("); }}; type__->RegisterMethod(\"");
    out.Append(gm->GetMeta().GetString(GlueMeta::NameName));
    out.Append("\",new M());}\n");
  }

  return true;
}

void GlueGen::WritePropertyAttributes (GlueProperty* gp, StringBuilder& out) {
  out.Append('0');

  if(gp->GetMeta().GetAndRemoveBool(Names::Private,false)) out.Append("|PropertyAttributes::Private");
  if(gp->GetMeta().GetAndRemoveBool(Names::ControlPart,false)) out.Append("|PropertyAttributes::ControlPart");

  if(!(gp->GetMeta().Contains(GlueMeta::NameNonSerialized))) out.Append("|PropertyAttributes::Serializable");
  if(!(gp->GetMeta().Contains(GlueMeta::NameWriteOnly))) out.Append("|PropertyAttributes::Read");
  if(!(gp->GetMeta().Contains(GlueMeta::NameReadOnly))) out.Append("|PropertyAttributes::Write");
}

void GlueGen::WritePropertyClassHeader (GlueClass* gc, GlueProperty* gp, const Name& base_name, StringBuilder& out) {
  out.Append("  {class P : public ");
  out.Append(base_name);
  out.Append(" { public: P (void) : ");
  out.Append(base_name);
  out.Append(" (");
  WritePropertyAttributes(gp,out);
  out.Append(") {} ");

  String serialize = gp->GetMeta().GetAndRemoveString("Serialize");
  if(!serialize.IsEmpty()) {
    out.Append ("virtual bool Serialize(Object* t, Serializer& s) {return ((");
    out.Append (gc->GetQualifiedName());
    out.Append ("*)t)->" );
    out.Append (serialize);
    out.Append ("(s);}");
  }

  String deserialize = gp->GetMeta().GetAndRemoveString("Deserialize");
  if(!deserialize.IsEmpty()) {
    out.Append ("virtual bool Deserialize(Object* t, Deserializer& s) {return ((");
    out.Append (gc->GetQualifiedName());
    out.Append ("*)t)->" );
    out.Append (deserialize);
    out.Append ("(s);}");
  }

  String isdefault = gp->GetMeta().GetAndRemoveString("IsDefault");
  if(!isdefault.IsEmpty()) {
    out.Append ("virtual bool IsDefault (Object* t) const {return ((");
    out.Append (gc->GetQualifiedName());
    out.Append ("*)t)->" );
    out.Append (isdefault);
    out.Append ("();}");
  }

}

void GlueGen::WritePropertyClassFooter(GlueProperty* gp, StringBuilder& out) { 
  out.Append("};P* p=new P(); ");
  for(auto it=gp->GetMeta().GetValues().begin(); it!=gp->GetMeta().GetValues().end(); it++) {
    if(GlueMeta::IsReservedName(it->first)) continue;
    out.Append("p->SetMeta(\"");
    out.Append(it->first.ToCString());
    out.Append("\",\"");
    out.Append(it->second.ToCString());
    out.Append("\");");
  }
  out.Append("type__->RegisterProperty(\"");
  out.Append(gp->GetMeta().GetString(GlueMeta::NameName));
  out.Append("\",p);}\n");
}


void GlueGen::WritePropertyObjectPtrVector(GlueClass* gc, GlueProperty* gp, const Name& v_type, StringBuilder& out) {
  WritePropertyClassHeader(gc,gp, "ObjectPtrVectorProperty", out);

  String t_cast = String::Format("((%s*)t)->", gc->GetQualifiedName().ToCString());      
  String v_cast = String::Format("((%s*)v)", v_type.ToCString());

  // Add
  String add = gp->GetMeta().GetAndRemoveString(Names::Add);
  out.Append("virtual void Add (Object* t, Object* v) override {");
  out.Append(t_cast);
  if(add.IsEmpty()) {
    out.Append(gp->GetName());
    out.Append(".push_back");
  } else {
    out.Append(add);
  }
  out.Append("(");
  out.Append(v_cast);
  out.Append(");}");
  

  // Clear
  String clear = gp->GetMeta().GetAndRemoveString(Names::Clear);
  out.Append("virtual void Clear (Object* t) override {");
  out.Append(t_cast);
  if(clear.IsEmpty()) {
    out.Append(gp->GetName());
    out.Append(".clear");
  } else {
    out.Append(clear);
  }
  out.Append("();}");

  // Get
  String get = gp->GetMeta().GetAndRemoveString(Names::Get);
  out.Append("virtual Object* Get (Object* t, noz_uint32 i) const override {return ");
  out.Append(t_cast);
  if(get.IsEmpty()) {
    out.Append(gp->GetName());
    out.Append("[i]");
  } else {
    out.Append(get);
    out.Append("(i)");
  }
  out.Append(";}");

  // GetCount
  String get_count = gp->GetMeta().GetAndRemoveString(Names::GetCount);
  out.Append("virtual noz_uint32 GetCount (Object* t) const override {return ");
  out.Append(t_cast);
  if(get_count.IsEmpty()) {
    out.Append(gp->GetName());
    out.Append(".size");
  } else {
    out.Append(get_count);
  }
  out.Append("();}");

  // Insert
  String insert = gp->GetMeta().GetAndRemoveString(Names::Insert);
  out.Append("virtual void Insert (Object* t, noz_uint32 i, Object* v) override {");
  out.Append(t_cast);
  if(insert.IsEmpty()) {
    out.Append(gp->GetName());
    out.Append(".insert(");
    out.Append(t_cast);
    out.Append(gp->GetName());
    out.Append(".begin()+i,");
    out.Append(v_cast);
  } else {
    out.Append(insert);
    out.Append("(i,");
    out.Append(v_cast);
  }
  out.Append(");}");

  // Remove
  String remove = gp->GetMeta().GetAndRemoveString(Names::Remove);
  out.Append("virtual void Remove (Object* t, noz_uint32 i) override {");
  out.Append(t_cast);
  if(remove.IsEmpty()) {
    out.Append(gp->GetName());
    out.Append(".erase(");
    out.Append(t_cast);
    out.Append(gp->GetName());
    out.Append(".begin()+i)");
  } else {
    out.Append(remove);
    out.Append("(i)");
  }
  out.Append(";}");

  // Release
  String release = gp->GetMeta().GetAndRemoveString(Names::Release);
  out.Append("virtual Object* Release (Object* t, noz_uint32 i) override {");
  if(release.IsEmpty()) {
    out.Append("Object* r=");
    out.Append(t_cast);
    out.Append(gp->GetName());
    out.Append("[i];");
    out.Append(t_cast);
    out.Append(gp->GetName());
    out.Append(".erase(");
    out.Append(t_cast);
    out.Append(gp->GetName());
    out.Append(".begin()+i);");
    out.Append("return r;");
  } else {
    out.Append("return ");
    out.Append(t_cast);
    out.Append(release);
    out.Append("(i);");
  }
  out.Append("}");

  // Reserve
  String reserve = gp->GetMeta().GetAndRemoveString(Names::Reserve);
  out.Append("virtual void Reserve (Object* t, noz_uint32 i) override {");
  out.Append(t_cast);
  if(reserve.IsEmpty()) {
    out.Append(gp->GetName());
    out.Append(".reserve(i)");
  } else {
    out.Append(reserve);
    out.Append("(i)");
  }
  out.Append(";}");
      
  // GetObjectType
  out.Append("virtual Type* GetObjectType (void) const override {return typeof(");
  out.Append(v_type);
  out.Append(");}");

  WritePropertyClassFooter(gp,out);
}

bool GlueGen::WriteProperties(GlueState* gs, GlueClass* gc, StringBuilder& out) {
  std::map<Name,Name> builtInMap;
  builtInMap["float"] = "Float";
  builtInMap["noz_float"] = "Float";
  builtInMap["int"] = "Int32";
  builtInMap["noz_int32"] = "Int32";
  builtInMap["noz_uint32"] = "UInt32";
  builtInMap["noz_byte"] = "Byte";
  builtInMap["noz::Color"] = "Color";
  builtInMap["bool"] = "Boolean";

  std::map<Name,Name> builtInMapGetParamType;

  std::map<Name,Name> builtInMapSetParamType;

  // Cache glue classes for specific types..
  GlueClass* classProperty = gs->GetQualifiedClass("noz::Property");
  GlueClass* classMethod = gs->GetQualifiedClass("noz::Method");
  GlueClass* classType = gs->GetQualifiedClass("noz::Type");
  GlueClass* classString = gs->GetQualifiedClass("noz::String");
  GlueClass* classObjectPtr = gs->GetQualifiedClass("noz::ObjectPtr");
  GlueClass* classObject = gs->GetQualifiedClass("noz::Object");
  GlueClass* classList = gs->GetQualifiedClass("std::list");
  GlueClass* classVector = gs->GetQualifiedClass("std::vector");
  GlueClass* className = gs->GetQualifiedClass("noz::Name");
  GlueClass* classNodePath = gs->GetQualifiedClass("noz::NodePath");
  GlueClass* classPropertyPath = gs->GetQualifiedClass("noz::PropertyPath");
  GlueClass* classColor = gs->GetQualifiedClass("noz::Color");
  GlueClass* classIChildren = gs->GetQualifiedClass("noz::IChildren");

  // Process all properties for the class..
  for(auto it=gc->GetProperties().begin(); it!= gc->GetProperties().end(); it++) {
    GlueProperty* gp = *it;
    noz_assert(gp);
    
    // Create a string that casts the target pointer to the class pointer
    String target_cast = String::Format("((%s*)t)->", gc->GetQualifiedName().ToCString());

    // Create a string for the variable name within the class.
    String variable_name;
    if(gp->IsArray() && gp->GetMeta().Contains(GlueMeta::NameElement)) {
      variable_name = String::Format("%s[%s]", gp->GetName().ToCString(), gp->GetMeta().GetString(GlueMeta::NameElement).ToCString());
    } else {
      variable_name = gp->GetName();
    }

    // Create a string for setting the property
    String set_value;
    if(!gp->GetMeta().Contains(GlueMeta::NameReadOnly)) {
      String set = gp->GetMeta().GetString(GlueMeta::NameSet);
      if(set.IsEmpty()) {
        set_value = String::Format("%s%s = v;", target_cast.ToCString(), variable_name.ToCString());
      } else {
        set_value = String::Format("%s%s(v);", target_cast.ToCString(), set.ToCString());
      }
    }

    // Create a string for getting the property
    String get_value;
    if(!gp->GetMeta().Contains(GlueMeta::NameWriteOnly)) {
      String get = gp->GetMeta().GetString(GlueMeta::NameGet);
      if(get.IsEmpty()) {
        get_value = String::Format("%s%s;", target_cast.ToCString(), variable_name.ToCString());
      } else {
        get_value = String::Format("%s%s();", target_cast.ToCString(), get.ToCString());
      }
    }

    // IChildren*
    if(gp->GetType()->IsClassPointer(classIChildren)) {
      WritePropertyClassHeader(gc,gp, "ChildrenProperty",out);
      String get = gp->GetMeta().GetString(GlueMeta::NameGet);
      if(!get.IsEmpty()) {
        out.Append("virtual IChildren* Get(Object* t) const override {return ");
        out.Append(String::Format("%s%s();", target_cast.ToCString(), get.ToCString()));
        out.Append("}");
      }
      WritePropertyClassFooter(gp,out);
      continue;      
    }

    // Fixed Array
    if(gp->IsArray() && !gp->GetMeta().Contains(GlueMeta::NameElement)) {
      out.Append("{class P : public FixedArrayPropertyT<");
      out.Append(gp->GetType()->GetRaw());
      out.Append("> { ");
      out.Append("public: P(void) : FixedArrayPropertyT(");
      out.Append(Int32(gp->GetArrayRank()).ToString());
      out.Append("){} virtual");
      out.Append(gp->GetType()->GetRaw());
      out.Append("* Get(Object* t) { return &(");
      out.Append(target_cast);
      out.Append(variable_name);
      out.Append("[0]);}");
      WritePropertyClassFooter(gp,out);
      continue;
    }

    // Built-in
    if(builtInMap.find(gp->GetType()->GetRaw()) != builtInMap.end()) {
      WritePropertyClassHeader(gc,gp,String::Format("%sProperty", builtInMap[gp->GetType()->GetRaw()].ToCString()),out);

      if(!get_value.IsEmpty()) {
        String get_param_type = builtInMapGetParamType[gp->GetType()->GetRaw()];
        if(get_param_type.IsEmpty()) get_param_type = gp->GetType()->GetRaw();
        out.Append("virtual ");
        out.Append(get_param_type);
        out.Append(" Get(Object *t) const override {return ");
        out.Append(get_value);
        out.Append("}");
      }

      if(!set_value.IsEmpty()) {
        String set_param_type = builtInMapSetParamType[gp->GetType()->GetRaw()];
        if(set_param_type.IsEmpty()) set_param_type = gp->GetType()->GetRaw();
        out.Append("virtual void Set(Object* t,");
        out.Append(set_param_type);
        out.Append(" v) override {");
        out.Append(set_value);
        out.Append("}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }

    // Enumeration
    if (gp->GetType()->IsEnum()) {
      GlueEnum* ge = gp->GetType()->GetEnum();
      noz_assert(ge);

      WritePropertyClassHeader(gc,gp,String::Format("EnumPropertyT<%s>", ge->GetQualifiedName().ToCString()),out);

      if(!get_value.IsEmpty()) {
        out.Append("virtual ");
        out.Append(ge->GetQualifiedName());
        out.Append(" GetRaw(Object* t) const override {return ");
        out.Append(get_value);
        out.Append("}");
      }

      if(!set_value.IsEmpty()) {
        out.Append("virtual void SetRaw(Object* t,");
        out.Append(ge->GetQualifiedName());
        out.Append(" v) override {");
        out.Append(set_value);
        out.Append("}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }

    // String 
    if(gp->GetType()->IsClass(classString) && !gp->GetType()->IsPointer()) {     
      WritePropertyClassHeader(gc,gp, "StringProperty",out);

      if(!get_value.IsEmpty()) {
        out.Append("virtual String Get(Object* t) const override {return ");
        out.Append(get_value);
        out.Append("}");
      }

      if(!set_value.IsEmpty()) {
        out.Append("virtual void Set(Object* t, const String& v) override {");
        out.Append(set_value);
        out.Append("}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }

    // Name
    if(gp->GetType()->IsClass(className) && !gp->GetType()->IsPointer()) {     
      WritePropertyClassHeader(gc,gp, "NameProperty",out);
      
      if(!get_value.IsEmpty()) {
        out.Append("virtual const Name& Get(Object* t) const override {return ");
        out.Append(get_value);
        out.Append("}");
      }

      if(!set_value.IsEmpty()) {
        out.Append("virtual void Set(Object* t, const Name& v) override {");
        out.Append(set_value);
        out.Append("}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }


    // PropertyProperty
    if (gp->GetType()->IsCastableTo(classProperty) && gp->GetType()->IsPointer()) {
      WritePropertyClassHeader(gc,gp, "PropertyProperty",out);

      if(!get_value.IsEmpty()) {
        out.Append("virtual Property* Get(Object* t) const override {return ");
        out.Append(get_value);
        out.Append("}");
      }

      if(!set_value.IsEmpty()) {
        out.Append("virtual void Set(Object* t, Property* _v) override {");
        out.Append(gp->GetType()->GetRaw());
        out.Append(" v=Cast<");
        out.Append(gp->GetType()->GetClass()->GetQualifiedName());
        out.Append(">(_v);");
        out.Append(set_value);
        out.Append("}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }

    // MethodProperty
    if (gp->GetType()->IsCastableTo(classMethod) && gp->GetType()->IsPointer()) {
      WritePropertyClassHeader(gc,gp, "MethodProperty",out);

      if(!get_value.IsEmpty()) {
        out.Append("virtual Method* Get(Object* t) const override {return ");
        out.Append(get_value);
        out.Append("}");
      }

      if(!set_value.IsEmpty()) {
        out.Append("virtual void Set(Object* t, Method* _v) override {");
        out.Append(gp->GetType()->GetRaw());
        out.Append(" v=Cast<");
        out.Append(gp->GetType()->GetClass()->GetQualifiedName());
        out.Append(">(_v);");
        out.Append(set_value);
        out.Append("}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }
        
    // Object
    if(!gp->GetType()->IsPointer() && gp->GetType()->IsClass() && gp->GetType()->GetClass()->IsReflected()) { 
      WritePropertyClassHeader(gc,gp, "ObjectProperty",out);
      
      out.Append("virtual Type* GetObjectType(void) const override {return typeof(");
      out.Append(gp->GetType()->GetClass()->GetQualifiedName());
      out.Append(");}");

      if(!get_value.IsEmpty()) {
        out.Append("virtual Object* Get(Object* t) const override {return ");

        // Add an & to return the address of the object..
        if(!gp->GetMeta().Contains(GlueMeta::NameGet)) out.Append("&");
       
        out.Append(get_value);
        out.Append(";}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }

    // ObjectPtr<T>
    if(gp->GetType()->IsTemplate() && gp->GetType()->IsTemplate(classObjectPtr)) {
      WritePropertyClassHeader(gc,gp, "ObjectPtrProperty",out);
      
      out.Append("virtual Type* GetObjectType(void) const {return typeof(");
      out.Append(gp->GetType()->GetTemplateArgType()->GetRaw());
      out.Append(");}");

      if(!get_value.IsEmpty()) {
        out.Append("virtual Object* Get(Object* t) const override {return ");
        out.Append(get_value);
        out.Append("}");
      } 

      if(!set_value.IsEmpty()) {
        out.Append("virtual void Set(Object* t, Object* _v) override {");
        out.Append(gp->GetType()->GetTemplateArgType()->GetRaw());
        out.Append("* v=Cast<");
        out.Append(gp->GetType()->GetTemplateArgType()->GetRaw());
        out.Append(">(_v);");
        out.Append(set_value);
        out.Append("}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }

    // Object*
    if(gp->GetType()->IsPointer() && gp->GetType()->IsClass() && gp->GetType()->GetClass()->IsReflected()) {
      WritePropertyClassHeader(gc,gp, "ObjectPtrProperty",out);
      
      out.Append("virtual Type* GetObjectType(void) const {return typeof(");
      out.Append(gp->GetType()->GetClass()->GetQualifiedName());
      out.Append(");}");

      if(!get_value.IsEmpty()) {
        out.Append("virtual Object* Get(Object* t) const override {return ");
        out.Append(get_value);
        out.Append("}");
      } 

      if(!set_value.IsEmpty()) {
        out.Append("virtual void Set(Object* t, Object* _v) override {");
        out.Append(gp->GetType()->GetClass()->GetQualifiedName());
        out.Append("* v=Cast<");
        out.Append(gp->GetType()->GetClass()->GetQualifiedName());
        out.Append(">(_v);");
        out.Append(set_value);
        out.Append("}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }

    // std::vector
    bool std_vector = gp->GetType()->IsTemplate(classVector);

    // std::vector<T> 
    if(std_vector && !gp->GetType()->GetTemplateArgType()->IsPointer() && gp->GetType()->GetTemplateArgType()->IsClass() && gp->GetType()->GetTemplateArgType()->GetClass()->IsReflected()) {
      WritePropertyClassHeader(gc,gp, "ObjectVectorProperty", out);

      // AddElement
      String add = gp->GetMeta().GetString(GlueMeta::NameAdd);
      out.Append("virtual Object* AddElement(Object* t) override {");
      out.Append(target_cast);
      if(add.IsEmpty()) {
        out.Append(variable_name);
        out.Append(".emplace_back();");
        out.Append("return &");
        out.Append(target_cast);
        out.Append(variable_name);
        out.Append(".back();");
      } else {
        out.Append("return ");
        out.Append(add);
        out.Append("();");
      }
      
      out.Append("}");

      // GetElement
      String get = gp->GetMeta().GetString(GlueMeta::NameGet);
      out.Append("virtual Object* GetElement(Object* t,noz_uint32 i) const override { return ");      
      if(get.IsEmpty()) {
        out.Append("&");
        out.Append(target_cast);
        out.Append(variable_name);
        out.Append("[i];}");
      } else {
        out.Append(target_cast);
        out.Append(get);
        out.Append("(i);}");
      }

      // GetSize
      String get_size = gp->GetMeta().GetString(GlueMeta::NameGet);
      out.Append("virtual noz_uint32 GetSize (Object* t) const override { return ");
      out.Append(target_cast);
      if(get_size.IsEmpty()) {
        out.Append(variable_name);
        out.Append(".size();}");
      } else {
        out.Append(get_size);
        out.Append("();}");
      }

      // SetSize
      String set_size = gp->GetMeta().GetString(GlueMeta::NameSetSize);
      out.Append("virtual void SetSize (Object* t, noz_uint32 size) override {");
      out.Append(target_cast);
      if(set_size.IsEmpty()) {
        out.Append(variable_name);
        out.Append(".reserve(size);}");
      } else {
        out.Append(set_size);
        out.Append("(size);}");
      }

      out.Append("virtual Type* GetElementType (void) const override {return typeof(");
      out.Append(gp->GetType()->GetTemplateArgType()->GetRaw());
      out.Append(");}");

      WritePropertyClassFooter(gp,out);
      continue;
    }

    // std::vector<T*>
    if (std_vector && gp->GetType()->GetTemplateArgType()->IsPointer() && gp->GetType()->GetTemplateArgType()->GetClass()->IsSubclassOf(classObject)) {
      WritePropertyObjectPtrVector(gc,gp,gp->GetType()->GetTemplateArgType()->GetClass()->GetQualifiedName(),out);
      continue;
    }

    // std::vector<ObjectPtr<T>>
    if (std_vector && !gp->GetType()->GetTemplateArgType()->IsPointer() && gp->GetType()->GetTemplateArgType()->IsTemplate(classObjectPtr)) {      
      WritePropertyObjectPtrVector(gc,gp,gp->GetType()->GetTemplateArgType()->GetTemplateArgType()->GetClass()->GetQualifiedName(),out);
      continue;
    }

    // std::vector<noz_byte>
    if (std_vector && !gp->GetType()->GetTemplateArgType()->GetRaw().CompareTo("noz_byte")) {
      WritePropertyClassHeader(gc,gp, "ByteVectorProperty",out);
      out.Append("virtual std::vector<noz_byte>& Get(Object* t) override {return ");
      out.Append(get_value);
      out.Append("}");
      WritePropertyClassFooter(gp,out);
      continue;
    }

    // std::vector<String>
    if (std_vector && gp->GetType()->GetTemplateArgType()->IsClass(classString)) {
      WritePropertyClassHeader(gc,gp, "StringVectorProperty",out);
      out.Append("virtual std::vector<String>& Get(Object* t) override {return ");
      out.Append(get_value);
      out.Append("}");
      WritePropertyClassFooter(gp,out);
      continue;
    }

    // TypeProperty
    if (gp->GetType()->IsClassPointer(classType)) {
      WritePropertyClassHeader(gc,gp, "TypeProperty",out);

      if(!get_value.IsEmpty()) {
        out.Append("virtual Type* Get(Object* t) const override {return ");
        out.Append(get_value);
        out.Append("}");
      }

      if(!set_value.IsEmpty()) {
        out.Append("virtual void Set(Object* t, Type* v) override {");
        out.Append(set_value);
        out.Append("}");
      }

      WritePropertyClassFooter(gp,out);
      continue;
    }

    // Automatic Properties
    if(gp->GetType()->IsClass() && !gp->GetType()->IsPointer()) {
      String auto_property_name = String::Format("%sProperty",gp->GetType()->GetClass()->GetName().ToCString());
      GlueClass* gc = gs->GetClass(auto_property_name);
      if (gc) {
        WritePropertyClassHeader(gc,gp, auto_property_name, out);
      
        if(!get_value.IsEmpty()) {
          out.Append("virtual const ");
          out.Append(gp->GetType()->GetRaw());
          out.Append("& Get(Object* t) const override {return ");
          out.Append(get_value);
          out.Append("}");
        }

        if(!set_value.IsEmpty()) {
          out.Append("virtual void Set(Object* t, const ");
          out.Append(gp->GetType()->GetRaw());
          out.Append("& v) override {");
          out.Append(set_value);
          out.Append("}");
        }

        WritePropertyClassFooter(gp,out);
        continue;
      }
    }

    ReportError(gp->GetFile(), gp->GetLineNumber(), "unknown property type '%s'", gp->GetType()->GetRaw().ToCString());
    return false;
  }

  return true;
}

bool GlueGen::WriteRegisterTypes (GlueState* gs, StringBuilder& out) {
  // The name of the register function is the name stripped of all extensions.  This
  // works because we name glue files using the system "<name>.<config>.glue.cpp"
  String name = Path::GetFilename(gs->GetOutputPath());
  noz_int32 dot = name.IndexOf('.');
  if(dot) name = name.Substring(0,dot);

  out.Append("void ");
  out.Append(name);
  out.Append("_RegisterTypes(void) {\n");

  for(auto it=gs->GetClasses().begin(); it!=gs->GetClasses().end(); it++) {
    GlueClass* gc = *it;
    noz_assert(gc);

    // Do not write out excluded classes
    if(gc->IsExcluded()) continue;

    // Do not write out template classes
    if(gc->IsTemplate()) continue;

    // Do not write nested classes here.
    if(gc->IsNested()) continue;

    out.Append("  ");
    out.Append(gc->GetQualifiedName());
    out.Append("::RegisterType();\n");
  }

  out.Append ("\n");

  if(!WriteEnums(gs,out)) return false;

  out.Append("}\n");

  return true;
}

bool GlueGen::WriteClasses (GlueState* gs, StringBuilder& out) {
  // Cache class pointers for base class testing.
  GlueClass* classObject = gs->GetQualifiedClass("noz::Object");
  GlueClass* classAsset = gs->GetQualifiedClass("noz::Asset");
  GlueClass* classControl = gs->GetQualifiedClass("noz::Control");
  noz_assert(classObject);
  noz_assert(classAsset);

  // Write all classes
  for(size_t it=0; it<gs->GetClasses().size(); it++) {
    GlueClass* gc = gs->GetClasses()[it];
    noz_assert(gc);

    // Skip excluded classes
    if(gc->IsExcluded()) continue;

    // Skip template classes
    if(gc->IsTemplate()) continue;

    // Output the class
    out.Append("void ");
    out.Append(gc->GetQualifiedName());
    out.Append("::RegisterType(void) {\n");
    out.Append("  type__ = new Type(\"");
    out.Append(gc->GetQualifiedName());
    out.Append("\",");

    // Attributes...
    if(gc->GetMeta().Contains(GlueMeta::NameManaged)) out.Append("TypeAttributes::Managed|");
    out.Append("0,");

    // TypeCode
    out.Append("TypeCode::");
    if(gc->GetMeta().Contains(GlueMeta::NameTypeCode)) {
      out.Append(gc->GetMeta().GetString(GlueMeta::NameTypeCode));
    } else if(gc->IsSubclassOf(classAsset)) {
      out.Append("Asset");
    } else if(gc->IsSubclassOf(classObject)) {
      out.Append("Object");
    } else {
      out.Append("Class");
    }

    // Base class
    out.Append(',');

    if(gc->GetNonTemplateBase()) {
      out.Append("typeof(");
      out.Append (gc->GetNonTemplateBase()->GetQualifiedName());
      out.Append(")");
    } else {
      out.Append("nullptr");
    }

    out.Append(");\n");

    // Allocator?
    if(!gc->IsAbstract() && gc->HasDefaultConstructor()) {
      // If the class is derived from "noz::Control" and contains a "DefaultTemplate" meta...
      if(gc->IsSubclassOf(classControl) && gc->GetMeta().Contains(Names::DefaultTemplate)) {
        Guid guid = Guid::Parse(gc->GetMeta().GetAndRemoveString(Names::DefaultTemplate));
        out.Append("  {class A : public ObjectAllocator {public: virtual Object* CreateInstance(ObjectAllocatorAttributes attr) override {");
        out.Append("if(attr&ObjectAllocatorAttributes::NoDefaultTemplate) return new ");
        out.Append(gc->GetQualifiedName());
        out.Append("; static const Guid guid = Guid::Parse(\"");
        out.Append(guid.ToString());
        out.Append("\"); noz::ControlTemplate* ct=AssetManager::LoadAsset<noz::ControlTemplate>(guid);");
        out.Append("if(ct) return ct->CreateControl<");
        out.Append(gc->GetQualifiedName());
        out.Append(">();return nullptr;}};");
        out.Append("type__->SetAllocator(new A);}\n");
      } else {
        out.Append("  {class A : public ObjectAllocator {public: virtual Object* CreateInstance(ObjectAllocatorAttributes attr) override { return (Object*) new ");
        out.Append(gc->GetQualifiedName());
        out.Append("();}};");
        out.Append("type__->SetAllocator(new A);}\n");
      }
    }

    // Write properties for the class
    if(!WriteProperties(gs,gc,out)) return false;

    // Write methods for the class
    if(!WriteMethods(gs,gc,out)) return false;

/* TODO: interfaces
    // Register all interfaces..
    for(auto it_interface=gc->interfaces.begin(); it_interface!=gc->interfaces.end(); it_interface++) {
      out += "  type__->RegisterInterface(typeof(" + (*it_interface)->qualified_name + "),";
      out += "[] (Object* o) -> void* {return (void*)(" + (*it_interface)->qualified_name + "*)(" + gc->qualified_name + "*)o;}";      
      out += ");\n";
    }
*/


    // Meta..
    for(auto itmeta=gc->GetMeta().GetValues().begin(); itmeta!=gc->GetMeta().GetValues().end(); itmeta++) {
      // skip reserved meta names
      if(GlueMeta::IsReservedName(itmeta->first)) continue;
     
      out.Append("  type__->RegisterMeta(\"");
      out.Append(itmeta->first);
      out.Append("\",\"");
      out.Append(itmeta->second);
      out.Append("\");\n");
    }

    // Register the type itself.
    out.Append("  Type::RegisterType(type__);\n");

    // Register nested classes
    for(auto it=gs->GetClasses().begin(); it!=gs->GetClasses().end(); it++) {
      GlueClass* gcc = *it;
      noz_assert(gcc);

      // Only write nested classes here.
      if(gcc->GetNestedParent() != gc) continue;

      // Do not write out excluded classes
      if(gcc->IsExcluded()) continue;

      // Do not write out template classes
      if(gcc->IsTemplate()) continue;

      out.Append("  ");
      out.Append(gcc->GetQualifiedName());
      out.Append("::RegisterType();\n");
    }

    out.Append("}\n\n");
  }

  return true;
}


bool GlueGen::WriteStaticVariables (GlueState* gs, StringBuilder& out) {  
  // Write static variables for classes
  for(size_t it=0; it<gs->GetClasses().size(); it++) {
    GlueClass* gc = gs->GetClasses()[it];
    noz_assert(gc);

    // Skip excluded files.
    if(gc->IsExcluded()) continue;

    // Skip templates
    if(gc->IsTemplate()) continue;

    out.Append("noz::Type* ");
    out.Append(gc->GetQualifiedName());
    out.Append("::type__ = nullptr;\n");
  }

  out.Append('\n');

  return true;
}

bool GlueGen::WriteIncludes(GlueState* gs, StringBuilder& out) {
  // Track the includes that have been written already
  std::set<GlueFile*> written;  

  String output_dir = Path::GetDirectoryName(gs->GetOutputPath());

  // For each class write out the include file it came from.
  for(size_t it=0; it<gs->GetClasses().size(); it++) {
    GlueClass* gc = gs->GetClasses()[it];    
    noz_assert(gc);

    // If the class is not included then skip it
    if(gc->IsExcluded()) continue;

    // Was the include already written?
    if(written.find(gc->GetFile()) != written.end()) continue;

    // Mark the file as written.
    written.insert(gc->GetFile());

    String path;
    for(auto it_inc=gs->GetIncludeDirectories().begin(); it_inc!=gs->GetIncludeDirectories().end(); it_inc++) {
      String rel = Path::GetRelativePath(gc->GetFile()->GetFullPath(),*it_inc);
      if(!rel.IsEmpty() && (path.IsEmpty() || rel.GetLength() <= path.GetLength())) {
        path = rel;
      }
    }
/*

    String relative_path = gc->GetFile()->GetFullPath().Substring(0,output_dir.GetLength());
    String path;
    if(!relative_path.CompareTo(output_dir)) {
      path = gc->GetFile()->GetFullPath().Substring(output_dir.GetLength()+1);
    } else {
      path = gc->GetFile()->GetPath();
    }
*/

    // Append the include directive
    out.Append ("#include <");
    out.Append (path);
    out.Append (">\n");
  }

  out.Append("\nusing namespace noz;\n\n");


  return true;
}


bool GlueGen::WriteHeader (GlueState* gs,StringBuilder& out) {
  out.Append("///////////////////////////////////////////////////////////////////////////////\n");
  out.Append("// NoZ Engine Glue File\n");
  out.Append("//\n");
  out.Append("// This file is automatically generated by noz_glue and should not be modified\n");
  out.Append("// \n");
  out.Append("// Source: ");
  out.Append(gs->GetProjectPath());
  out.Append("\n");
  out.Append("// Config: ");
  out.Append(gs->GetConfigName());
  out.Append("\n");
  out.Append("///////////////////////////////////////////////////////////////////////////////\n");
  out.Append("\n");

  if(gs->GetPrecompiledHeader()) {
    out.Append("#include <");
    out.Append(Path::GetFilename(gs->GetPrecompiledHeader()->GetFullPath()));
    out.Append(">\n\n");
  }

  return true;
}

bool GlueGen::Write (GlueState* gs) {
  // Open target file
  FileStream fs;
  if(!fs.Open(gs->GetOutputPath(),FileMode::Truncate)) {
    Console::WriteLine ("%s: error: failed to open target file for writing", gs->GetOutputPath().ToCString());
    return false;
  }

  // Generate output..
  StringBuilder out;
  if(!WriteHeader(gs,out)) return false;
  if(!WriteIncludes(gs,out)) return false;
  if(!WriteStaticVariables(gs,out)) return false;
  if(!WriteClasses(gs,out)) return false;
  if(!WriteRegisterTypes(gs,out)) return false;

  // Write the output to the file.
  String write = out.ToString();
  fs.Write((char*)write.ToCString(), 0, write.GetLength());    

  return true;
}

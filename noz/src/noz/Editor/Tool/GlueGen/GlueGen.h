///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueGen_h__
#define __noz_Editor_GlueGen_h__

using namespace std;

#include <noz/Editor/Tool/Makefile.h>

#include "GlueState.h"

namespace noz {
namespace Editor {

  class GlueGen {
    public: struct Options {
      String config_;
      String output_path_;
      bool clean_;
      std::set<Name> defines_;
      Makefile::PlatformType platform_;
    };

    private: struct ParseState {
      GlueState* gs;
      GlueFile* gf;
      GlueLexer* lexer;
      GlueLexer::TokenType access;
      std::vector<Name> namespace_stack_;

      ParseState (void);

      ~ParseState(void);

      void PushNamespace (const Name& ns);

      void PopNamespace (void);

      const Name& GetNamespace (void) const;

      GlueClass* GetClassInNamespace (const Name& name) const;

      GlueEnum* GetEnumInNamespace (const Name& name) const;

      GlueEnum* ResolveEnum (const Name& name);

      GlueClass* ResolveClass (const Name& name);

      void ReportError (const char* format, ...);
    };

  	public: static void ReportError (GlueFile* gf, noz_int32 line, const char* format, ...);

    public: static bool Generate (const String& project_path, const Options& options);

    private: static bool SetOptions (GlueState* gs, const String& project_path, const Options& options); 

    private: static bool ParseIncludedFiles (ParseState* ps);
    private: static bool ParseIncludedFiles (GlueState* gs);
    private: static const char* ParseWhitespace (const char* p);
    private: static const char* ParseUntil (const char* p, char c);
    private: static void ParseMeta(ParseState* ps, GlueMeta& meta);
    private: static Name ParseQualifiedName(ParseState* ps, GlueLexer* lexer);
    private: static Name ParseQualifiedName(ParseState* ps);
    private: static GlueType* ParseType(ParseState* ps, const Name& name, bool is_pointer, bool is_reference);
    private: static GlueType* ParseType(ParseState* ps, GlueLexer* lexer);
    private: static GlueType* ParseType(ParseState* ps);
    private: static bool ParseEnum(ParseState* ps);
    private: static GlueClass* ParseClass(ParseState* ps);
    private: static void ParseTemplate(ParseState* ps);
    private: static bool ParseNamespace(ParseState* ps);
    private: static bool ParseParameters(ParseState* ps, GlueMethod* gm);
    private: static bool ParseConstructor(ParseState* ps, GlueClass* gc, GlueMethod* gm);
    private: static void ParseProperty(ParseState* ps, GlueClass* gc);
    private: static bool ParseMethod(ParseState* ps, GlueClass* gc);
    private: static bool Parse (GlueState* gs);
    private: static bool ParseDeclarations(ParseState* ps, GlueClass* nested_parent=nullptr);
    private: static bool ParseDeclarations(GlueState* gs);

    private: static bool WriteEnums(GlueState* gs, StringBuilder& out);
    private: static bool WriteMethods(GlueState* gs, GlueClass* gc, StringBuilder& out);
    private: static void WritePropertyAttributes (GlueProperty* gp, StringBuilder& out);
    private: static void WritePropertyClassHeader (GlueClass* gc, GlueProperty* gp, const Name& base_name, StringBuilder& out);
    private: static void WritePropertyClassFooter(GlueProperty* gp, StringBuilder& out);
    private: static void WritePropertyObjectPtrVector(GlueClass* gc, GlueProperty* gp, const Name& v_type, StringBuilder& out);
    private: static bool WriteProperties(GlueState* gs, GlueClass* gc, StringBuilder& out);
    private: static bool WriteRegisterTypes (GlueState* gs, StringBuilder& out);
    private: static bool WriteClasses (GlueState* gs, StringBuilder& out);
    private: static bool WriteStaticVariables (GlueState* gs, StringBuilder& out);
    private: static bool WriteIncludes(GlueState* gs, StringBuilder& out);
    private: static bool WriteHeader (GlueState* gs,StringBuilder& out);
    private: static bool Write (GlueState* gs);
  };

} // namespace Editor
} // namespace noz


#endif // __noz_Editor_GlueGen_h__



///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "GlueGen.h"

using namespace noz;
using namespace noz::Editor;

GlueGen::ParseState::ParseState (void) {
  lexer = nullptr;
  access = GlueLexer::TokenType::Private;
  gf = nullptr;
  gs = nullptr;
}

GlueGen::ParseState::~ParseState(void) {
  delete lexer;
}

void GlueGen::ParseState::PushNamespace (const Name& ns) {
  if(namespace_stack_.empty()) {
    namespace_stack_.push_back(ns);
  } else {
    namespace_stack_.push_back(String::Format("%s::%s", 
      namespace_stack_.back().ToCString(),
      ns.ToCString()
    ));
  }
}

void GlueGen::ParseState::PopNamespace (void) {
  namespace_stack_.pop_back();
}

const Name& GlueGen::ParseState::GetNamespace (void) const {
  if(namespace_stack_.empty()) return Name::Empty;
  return namespace_stack_.back();
}

GlueClass* GlueGen::ParseState::GetClassInNamespace (const Name& name) const {
  if(namespace_stack_.empty()) {
    return gs->GetQualifiedClass(name);
  }

  return gs->GetQualifiedClass(
    String::Format("%s::%s",
      namespace_stack_.back().ToCString(),
      name.ToCString()
    )
  );
}

GlueEnum* GlueGen::ParseState::GetEnumInNamespace (const Name& name) const {
  noz_assert(gs);

  if(namespace_stack_.empty()) {
    return gs->GetQualifiedEnum(name);
  }

  return gs->GetQualifiedEnum(
    String::Format("%s::%s",
      namespace_stack_.back().ToCString(),
      name.ToCString()
    )
  );
}

GlueEnum* GlueGen::ParseState::ResolveEnum (const Name& name) {
  return gs->ResolveEnum(name,namespace_stack_.empty()?Name::Empty:namespace_stack_.back());
}

GlueClass* GlueGen::ParseState::ResolveClass (const Name& name) {
  return gs->ResolveClass(name,namespace_stack_.empty()?Name::Empty:namespace_stack_.back());
}

void GlueGen::ParseState::ReportError (const char* format, ...) {
  char msg[2048];
  va_list args;	
	va_start(args,format);
#if defined(NOZ_WINDOWS)
  vsprintf_s(msg,2048, format,args);
#else
  vsprintf(msg,format,args);
#endif
  va_end(args);

  Console::WriteLine("%s(%d): error: %s", gf->GetFullPath().ToCString(), lexer->GetToken().pos->line, msg);
}

const char* GlueGen::ParseWhitespace (const char* p) {
  while(*p && (*p==' ' || *p=='\n' || *p=='\r' || *p=='\t')) p++;
  return p;
}

const char* GlueGen::ParseUntil (const char* p, char c) {
  while(*p && *p != c) p++;
  return p;
}

void GlueGen::ParseMeta(ParseState* ps, GlueMeta& meta) {
  noz_assert(ps);
  noz_assert(ps->lexer);

  GlueLexer* lexer = ps->lexer;

  if(lexer->Is(GlueLexer::TokenType::CloseParen)) {
    return;
  }

  do {
    String name;
    if(!lexer->ExpectIdentifier(&name)) {
      ps->ReportError("missing meta name");
      return;
    }

    // Must be assignment...
    String value;
    if(lexer->Expect(GlueLexer::TokenType::Assignment)) {
      // Parse until the next comma or closing paren.
      StringBuilder sb;
      while(!lexer->Is(GlueLexer::TokenType::Comma) && !lexer->Is(GlueLexer::TokenType::CloseParen)) {
        sb.Append(lexer->GetToken().ToString());
        lexer->Skip();
      }
      value = sb.ToString();
    }
    
    meta.Set(name,value);

  } while (lexer->Expect(GlueLexer::TokenType::Comma));
}

Name GlueGen::ParseQualifiedName(ParseState* ps, GlueLexer* lexer) {
  StringBuilder sb;

  // Name of type
  while(1) {
    String name;
    if(!lexer->ExpectIdentifier(&name)) {
      return Name::Empty;
    }

    sb.Append(name.Trim());
    
    // If not qualified any further then we are done
    if(!lexer->Expect(GlueLexer::TokenType::Scope)) {
      break;
    }

    sb.Append("::");
  }

  return sb.ToString();
}

Name GlueGen::ParseQualifiedName(ParseState* ps) {
  return ParseQualifiedName(ps,ps->lexer);
}

GlueType* GlueGen::ParseType(ParseState* ps, const Name& name, bool is_pointer, bool is_reference) {
  Name qualified_name;

  // Attempt to resolve as a class name
  GlueClass* gc = ps->ResolveClass(name);
  GlueEnum* ge = nullptr;
  if(nullptr==gc) {
    ge = ps->ResolveEnum(name);
    if(ge) {
      qualified_name = ge->GetQualifiedName();
    } else {
      qualified_name = name;
    }
  } else {
    qualified_name = gc->GetQualifiedName();
  }

  String raw;
  if(is_pointer) {
    raw = String::Format("%s*", qualified_name.ToCString());
  } else if (is_reference) {
    raw = String::Format("%s&", qualified_name.ToCString());
  } else {
    raw = qualified_name;
  }

  // Find the type..
  GlueType* gt = ps->gs->GetQualifiedType(raw);
  if(gt==nullptr) {
    return nullptr;
  }

  // If the type has not been populated yet then do so now.
  if(gt->GetRaw().IsEmpty()) {
    gt->SetPointer(is_pointer);
    gt->SetReference(is_reference);
    gt->SetRaw(raw);
    gt->SetClass(gc);
    gt->SetEnum(ge);
  }

  return gt;
}

GlueType* GlueGen::ParseType(ParseState* ps, GlueLexer* lexer) {
  // Const?
  bool is_const = lexer->Expect(GlueLexer::TokenType::Const);

  // Look up the base type
  Name baseName = ParseQualifiedName(ps,lexer);

  // Find base type
  GlueType* baseType = nullptr;
  if(lexer->Expect(GlueLexer::TokenType::Multiply)) {
    baseType = ParseType(ps,baseName,true,false);
  } else if(lexer->Expect(GlueLexer::TokenType::LogicalAND)) {
    baseType = ParseType(ps,baseName,false,true);
  } else {
    baseType = ParseType(ps,baseName,false,false);
  }

  if(baseType==nullptr) return nullptr;

  // Set const flag.
  baseType->SetConst(is_const);

  // Is this a template?
  if(lexer->Expect(GlueLexer::TokenType::LessThan)) {
    // Parse internal type.
    GlueType* argType = ParseType(ps,lexer);
    if(nullptr==argType) {
      return nullptr;
    }
        
    if(!lexer->Expect(GlueLexer::TokenType::GreaterThan)) {
      return nullptr;
    }

    // Temmplate type
    GlueType* tempType = new GlueType;
    tempType->SetRaw(String::Format("%s<%s>", baseType->GetRaw().ToCString(), argType->GetRaw().ToCString()));
    tempType->SetTemplate(baseType,argType);
    baseType = tempType;
  }

  return baseType;
}

GlueType* GlueGen::ParseType(ParseState* ps) {
  return ParseType(ps,ps->lexer);
}

bool GlueGen::ParseEnum(ParseState* ps) {
  noz_assert(ps);
  noz_assert(ps->lexer);

  GlueLexer* lexer = ps->lexer;

  // If the enum is preceeded by NOZ_ENUM then it is a reflected enum
  bool reflected = lexer->Expect(GlueLexer::TokenType::NOZ_ENUM);    

  // Parse the meta data if reflected
  GlueMeta meta;
  if(reflected) {
    if (!lexer->Expect(GlueLexer::TokenType::OpenParen)) return true;

    // Parse meta data..
    ParseMeta(ps, meta);

    if (!lexer->Expect(GlueLexer::TokenType::CloseParen)) {
      ps->ReportError("missing ')'");
      return false;
    }
  }

  if(!lexer->Expect(GlueLexer::TokenType::Enum)) return true;
  if(!lexer->Expect(GlueLexer::TokenType::Class)) return true;

  // Parse name.
  Name name;
  if(!lexer->ExpectIdentifier(&name)) return true;

  // TODO: support enums with a type

  if(!lexer->Expect(GlueLexer::TokenType::OpenBrace)) return true;

  // Find enum that was discovered in ParseDeclarations..
  GlueEnum* ge = ps->GetEnumInNamespace(name);
  if(nullptr==ge) {
    while(!lexer->IsEnd() && !lexer->Expect(GlueLexer::TokenType::OpenBrace)) lexer->Skip();
    return true;
  }

  ge->SetReflected(reflected);
  ge->SetExcluded(ps->gf->IsExcluded());

  int value = 0;
  while(!lexer->Is(GlueLexer::TokenType::CloseBrace)) {
    Name enum_name;
    lexer->ExpectIdentifier(&enum_name);

    if(lexer->Expect(GlueLexer::TokenType::Assignment)) {
      if(lexer->IsIdentifier()) {
        Name temp;
        lexer->ExpectIdentifier(&temp);

        if(ge->Contains(temp)) {
          value = ge->GetValue(temp);
        }
      } else if (lexer->Is(GlueLexer::TokenType::Minus) || lexer->IsLiteral()) {
        if(lexer->Expect(GlueLexer::TokenType::Minus)) {
          value = -Int32::Parse(lexer->GetToken().ToString());
        } else {
          value = Int32::Parse(lexer->GetToken().ToString());
        }
        lexer->Skip();
      }
    }

    ge->AddValue(enum_name,value);      

    value++;

    if(!lexer->Expect(GlueLexer::TokenType::Comma)) break;
  }  

  lexer->Expect(GlueLexer::TokenType::CloseBrace);

  return true;
}

GlueClass* GlueGen::ParseClass(ParseState* ps) {
  noz_assert(ps);
  noz_assert(ps->lexer);

  GlueLexer* lexer = ps->lexer;

  if(!lexer->Expect(GlueLexer::TokenType::Class) && !lexer->Expect(GlueLexer::TokenType::Struct)) {
    return nullptr;
  }

  Name name;
  if(!lexer->ExpectIdentifier(&name)) {
    return nullptr;
  }

  // Skip forward references.
  if(lexer->Expect(GlueLexer::TokenType::Semicolon)) {
    return nullptr;
  }

  // Find the class
  GlueClass* gc = ps->GetClassInNamespace(name);
  if(nullptr==gc) {
    return nullptr;
  }  
   
  // Base class?
  if(lexer->Expect(GlueLexer::TokenType::Colon)) {
    while(1) {
      if(!lexer->Expect(GlueLexer::TokenType::Public)) {
        lexer->Expect(GlueLexer::TokenType::Private);
      }

      // Name of base class
      Name base_name = ParseQualifiedName(ps);

      // Template?
      if(lexer->Expect(GlueLexer::TokenType::LessThan)) {
        do {
          ParseType(ps);
        } while (lexer->Expect(GlueLexer::TokenType::Comma));

        if(!lexer->Expect(GlueLexer::TokenType::GreaterThan)) {
          break;
        }
      }
      
      gc->AddBaseName(base_name);

      if(lexer->Expect(GlueLexer::TokenType::Comma)) {
        continue;
      }

      break;
    }
  }

  // Open brace..
  if(!lexer->Expect(GlueLexer::TokenType::OpenBrace)) {
    return gc;
  }

  // Push class as a namespace
  ps->PushNamespace(gc->GetName());

  // parse class internals...
  int depth=1;
  bool excluded = true;
  while(depth && !lexer->IsEnd()) {
    switch(lexer->GetToken().type) {
      default:
        break;
    
      case GlueLexer::TokenType::OpenBrace: 
        depth++;
        break;
    
      case GlueLexer::TokenType::CloseBrace:
        depth--;
        break;

      case GlueLexer::TokenType::NOZ_ENUM:
      case GlueLexer::TokenType::Enum:
        ParseEnum(ps);
        continue;

      case GlueLexer::TokenType::Template:
        ParseTemplate(ps);
        continue;

      case GlueLexer::TokenType::Class:
      case GlueLexer::TokenType::Struct:
        // Add the namespace to the namespace stack
        ParseClass(ps);
        continue;

      case GlueLexer::TokenType::Public:
      case GlueLexer::TokenType::Protected:
      case GlueLexer::TokenType::Private:
        if(lexer->IsNext(GlueLexer::TokenType::Colon)) {
          ps->access = lexer->GetToken().type;
          lexer->Skip();
        }
        break;

      case GlueLexer::TokenType::Assignment:
        if(depth==1) {
          lexer->Skip();
          if(lexer->Expect(GlueLexer::TokenType::LiteralInteger)) {
            if(lexer->Expect(GlueLexer::TokenType::Semicolon)) {
              gc->SetAbstract();
            }
          }
        } else {
          lexer->Skip();
        }
        continue;

      case GlueLexer::TokenType::NOZ_OBJECT_BASE:
      case GlueLexer::TokenType::NOZ_OBJECT:
      case GlueLexer::TokenType::NOZ_TEMPLATE:
      case GlueLexer::TokenType::NOZ_INTERFACE:
        // Since it was indicated that this class is a NOZ class we clear
        // the excluded flag providing the file it came from isnt excluded.
        gc->SetExcluded(ps->gf->IsExcluded());

        // Mark the class as a reflected class
        gc->SetReflected(true);

        // Interace?
        gc->SetInterface (lexer->GetToken().type == GlueLexer::TokenType::NOZ_INTERFACE);

        lexer->Skip();

        if(lexer->Expect(GlueLexer::TokenType::OpenParen)) {
          ParseMeta(ps,gc->GetMeta());
          lexer->Expect(GlueLexer::TokenType::CloseParen);
        }

        // Mark class as abstract if meta includes abstract
        if(gc->GetMeta().Contains(GlueMeta::NameAbstract)) {
          gc->SetAbstract();
        }

        continue;

      case GlueLexer::TokenType::NOZ_METHOD:
        ParseMethod(ps,gc);
        continue;

      case GlueLexer::TokenType::NOZ_PROPERTY:
      case GlueLexer::TokenType::NOZ_CONTROL_PART:
        ParseProperty(ps,gc);
        continue;

      case GlueLexer::TokenType::Tilde: {
        lexer->Skip();

        // Destructor?
        String identifier = lexer->GetToken().ToString();
        const String& s = name;
        if(!identifier.CompareTo(s)) {
          lexer->Skip();
        }
        continue;
      }

      case GlueLexer::TokenType::Identifier: {
        // Look for constructors.
        String identifier = lexer->GetToken().ToString();
        const String& s = name;
        if(!identifier.CompareTo(s)) {
          ParseMethod(ps,gc);
          continue;
        }
        
        break;
      }
    }

    lexer->Skip();
  }

  ps->PopNamespace();
  
  return gc;
}

void GlueGen::ParseTemplate(ParseState* ps) {
  noz_assert(ps);
  noz_assert(ps->lexer);

  GlueLexer* lexer = ps->lexer;

  if(!lexer->Expect(GlueLexer::TokenType::Template)) {
    return;
   }
   
  if(!lexer->Expect(GlueLexer::TokenType::LessThan)) {
    return;
  }  

  while(!lexer->Expect(GlueLexer::TokenType::GreaterThan)) {
    lexer->Skip();
  }

  GlueClass* gc = ParseClass(ps);
  if(gc) {
    gc->SetTemplate(true);
  }
}

bool GlueGen::ParseNamespace(ParseState* ps) {
  noz_assert(ps->lexer);

  GlueLexer* lexer = ps->lexer;
  int depth=1;

  for(;depth>0&&!lexer->IsEnd();) {
    String v = lexer->GetToken().ToString();
    switch(lexer->GetToken().type) {
      case GlueLexer::TokenType::Pound:
        lexer->Expect(GlueLexer::TokenType::Pound);
        if(lexer->Expect(GlueLexer::TokenType::Define)) {
          if(lexer->Expect(GlueLexer::TokenType::NOZ_ENUM)) continue;
          if(lexer->Expect(GlueLexer::TokenType::NOZ_OBJECT)) continue;
          if(lexer->Expect(GlueLexer::TokenType::NOZ_OBJECT_BASE)) continue;
          if(lexer->Expect(GlueLexer::TokenType::NOZ_TEMPLATE)) continue;
          if(lexer->Expect(GlueLexer::TokenType::NOZ_INTERFACE)) continue;
        }
        continue;

      case GlueLexer::TokenType::NOZ_ENUM:
      case GlueLexer::TokenType::Enum:
        ParseEnum(ps);
        continue;

      case GlueLexer::TokenType::Template:
        ParseTemplate(ps);
        break;

      case GlueLexer::TokenType::Class:
      case GlueLexer::TokenType::Struct:
        ParseClass(ps);
        break;

      case GlueLexer::TokenType::Using: {
        // TODO: handle using to find classes?
        while(!lexer->Expect(GlueLexer::TokenType::Semicolon)&&!lexer->IsEnd()) lexer->Skip();
        break;
      }

      case GlueLexer::TokenType::Namespace: {
        lexer->Skip();
        Name ns = lexer->GetToken().ToString();
        lexer->Skip();
        lexer->Expect(GlueLexer::TokenType::OpenBrace);

        // Add the namespace to the namespace stack
        ps->PushNamespace(ns);

        // Parse the new namespace
        ParseNamespace(ps);

        // Pop the namespace from the namespace stack
        ps->PopNamespace();
        break;
      }

      case GlueLexer::TokenType::CloseBrace:
        depth--;
        lexer->Skip();
        break;

      case GlueLexer::TokenType::OpenBrace:
        depth++;
        lexer->Skip();
        break;
        
      default:
        lexer->Skip();
        break;
    }
  }

  return true;
}

bool GlueGen::ParseParameters(ParseState* ps, GlueMethod* gm) {
  noz_assert(ps);
  noz_assert(ps->lexer);

  GlueLexer* lexer = ps->lexer;

  if(!lexer->Expect(GlueLexer::TokenType::OpenParen)) {
    return false;
  }

  // Parse the parameters
  if(!lexer->Expect(GlueLexer::TokenType::Void)) {
    while(!lexer->Is(GlueLexer::TokenType::CloseParen)) {
      GlueParameter* gp = new GlueParameter;
      gp->SetType(ParseType(ps,lexer));
      
      // Optional parameter name
      if(lexer->IsIdentifier()) {
        gp->SetName(lexer->GetToken().ToString());
        lexer->Skip();
      }

      // Optional default parameters
      if(lexer->Is(GlueLexer::TokenType::Assignment)) {
        gp->SetDefault(true);
      }

      // Add the paraemters.
      gm->AddParameter(gp);

      // Read till comma or close parent.
      while(!lexer->Expect(GlueLexer::TokenType::Comma) && !lexer->Is(GlueLexer::TokenType::CloseParen)) {
        lexer->Skip();
      }        
    }
  }

  return lexer->Expect(GlueLexer::TokenType::CloseParen);
}

bool GlueGen::ParseConstructor(ParseState* ps, GlueClass* gc, GlueMethod* gm) {
  noz_assert(ps);
  noz_assert(ps->lexer);

  GlueLexer* lexer = ps->lexer;

  // Skip the constructor name 
  lexer->Skip();

  // Parse the parameters
  if(!ParseParameters(ps,gm)) {
    return false;
  }

  // Mark as a constructor
  gm->SetConstructor(true);

  // Set the name to be the same as the class
  gm->SetName(gc->GetName());

  // Add the constructor to the class
  gc->AddConstructor(gm);

  return true;
}

void GlueGen::ParseProperty(ParseState* ps, GlueClass* gc) {
  noz_assert(ps);
  noz_assert(ps->lexer);

  GlueLexer* lexer = ps->lexer;

  bool control_part = false;
  if(lexer->Is(GlueLexer::TokenType::NOZ_CONTROL_PART)) {  
    control_part = true;
  }

  // skip NOZ_PROPERTY or NOZ_CONTROL_PART
  lexer->Skip();

  // Parse the meta data 
  if(!lexer->Expect(GlueLexer::TokenType::OpenParen)) return;

  // Allocate the new property
  GlueProperty* gp = new GlueProperty;
  gp->SetFile(ps->gf,lexer->GetToken().pos->line);

  // Parse meta data..
  ParseMeta(ps,gp->GetMeta());  

  if(control_part) {
    gp->GetMeta().Set("Private");
    gp->GetMeta().Set("NonSerialized");
    gp->GetMeta().Set("ControlPart");
  }

  if(!lexer->Expect(GlueLexer::TokenType::CloseParen)) {
    ps->ReportError("missing ')'");
    return;
  }
  
  // If type specified then we dont need to read the variable after.
  if(gp->GetMeta().Contains(GlueMeta::NameType)) {
    GlueLexer lexer(gp->GetMeta().GetString(GlueMeta::NameType).ToCString(), gp->GetMeta().GetString("Type").GetLength());
    gp->SetType(ParseType(ps,&lexer));
    gc->AddProperty(gp);
    return;
  }

  // Optional semi-colon
  lexer->Expect(GlueLexer::TokenType::Semicolon);

  // Optional access specifier
  if(lexer->Is(GlueLexer::TokenType::Private) || lexer->Is(GlueLexer::TokenType::Public) || lexer->Is(GlueLexer::TokenType::Protected)) {
    lexer->Skip();
    if(!lexer->Expect(GlueLexer::TokenType::Colon)) {
      return;
    }
  }

  // Type
  gp->SetType(ParseType(ps));

  // Name
  Name name;
  if(!lexer->ExpectIdentifier(&name)) {
    ps->ReportError("missing property field name");
    return;
  }

  gp->SetName(name);

  // If no name was given in meta then use the variable name.
  if(!gp->GetMeta().Contains(GlueMeta::NameName)) {
    gp->GetMeta().Set(GlueMeta::NameName,gp->GetName());
  }

  // Array?
  if(lexer->Expect(GlueLexer::TokenType::OpenBracket)) {
    // TODO: multi-dimensional?
    if(!lexer->Is(GlueLexer::TokenType::LiteralInteger)) {
      ps->ReportError("missing array size");
      return;
    }

    gp->SetArrayRank(Int32::Parse(lexer->GetToken().ToString()));

    lexer->Skip();
    if(!lexer->Expect(GlueLexer::TokenType::CloseBracket)) {
      ps->ReportError("missing ']'");
      return;
    }
  }

  gc->AddProperty(gp);
}

bool GlueGen::ParseMethod(ParseState* ps, GlueClass* gc) {
  noz_assert(ps);
  noz_assert(ps->lexer);

  GlueLexer* lexer = ps->lexer;

  GlueMethod* gm = new GlueMethod;
  gm->SetClass(gc);
  gm->SetAccess(ps->access);

  // optional NOZ_METHOD
  if(lexer->Expect(GlueLexer::TokenType::NOZ_METHOD)) {
    if(!lexer->Expect(GlueLexer::TokenType::OpenParen)) {
      delete gm;
      ps->ReportError("missing '('");
      return false;
    }

    // Read meta
    ParseMeta(ps, gm->GetMeta());

    if(!lexer->Expect(GlueLexer::TokenType::CloseParen)) {
      ps->ReportError("missing ')'");
      delete gm;
      return false;
    }

    gm->SetReflected(true);
  }

  // Optional accessiblity
  if(lexer->Expect(GlueLexer::TokenType::Private) || lexer->Expect(GlueLexer::TokenType::Public) || lexer->Expect(GlueLexer::TokenType::Protected)) {
    if(!lexer->Expect(GlueLexer::TokenType::Colon)) {
      ps->ReportError("missing ':'");
      delete gm;
      return false;
    }
  }      

  // Is the method static or virtual?
  gm->SetStatic(lexer->Is(GlueLexer::TokenType::Static));
  gm->SetVirtual(lexer->Is(GlueLexer::TokenType::Virtual));
  if(gm->IsStatic() || gm->IsVirtual() ) {
    lexer->Skip();
  }

  // Constructor ?
  if(lexer->IsIdentifier(gc->GetName().ToCString()) && lexer->IsNext(GlueLexer::TokenType::OpenParen)) {
    return ParseConstructor(ps,gc,gm);
  }

  // Get return type.
  if(!lexer->Expect(GlueLexer::TokenType::Void)) {
    gm->SetReturnType(ParseType(ps));
  } 

  Name name;
  lexer->ExpectIdentifier(&name);
  gm->SetName(name);

  // Fill in the name meta with the method name if none was given
  if(!gm->GetMeta().Contains(GlueMeta::NameName)) {
    gm->GetMeta().Set(GlueMeta::NameName,gm->GetName());
  }

  if(!ParseParameters(ps,gm)) {
    return false;
  }

  gm->SetConst(lexer->Expect(GlueLexer::TokenType::Const));

  if(lexer->Expect(GlueLexer::TokenType::Assignment)) {
    if(lexer->Expect(GlueLexer::TokenType::LiteralInteger)) {
      gm->SetAbstract(true);
      gc->SetAbstract ();
    }
  }

  gc->AddMethod(gm);

  return true;
}


bool GlueGen::Parse (GlueState* gs) {
  for(auto it=gs->GetFiles().begin(); it!=gs->GetFiles().end(); it++) {
    GlueFile* gf = *it;
    noz_assert(gf);

    // Reflected types must be in header files.
    if(gf->GetType () != GlueFileType::H) continue;

    ParseState ps;
    ps.gs = gs;
    ps.gf = gf;
    ps.lexer = gf->CreateLexer();

    if(!ParseNamespace(&ps)) return false;
  }

  return true;
}

bool GlueGen::ParseIncludedFiles (ParseState* ps) {
  noz_assert(ps);

  for(const char* p = ps->gf->GetData();*p;) {
    // Looking for #include..
    if(*p != '#') {p++; continue;}
    if(*(++p) != 'i') continue;
    if(*(++p) != 'n') continue;
    if(*(++p) != 'c') continue;
    if(*(++p) != 'l') continue;
    if(*(++p) != 'u') continue;
    if(*(++p) != 'd') continue;
    if(*(++p) != 'e') continue;

    // Skip whitespace
    p = ParseWhitespace(++p);

    // Quoted string?
    bool search_local = false;
    String path;
    if(*p == '\"') {
      search_local  = true;
      const char* s = p + 1;
      p = ParseUntil(s,'\"');
      path = String(s,0,p-s);

    // String in <>
    } else if (*p == '<') {
      const char* s = p + 1;
      p = ParseUntil(s,'>');
      path = String(s,0,p-s);

    // Unknown, just skip..
    } else {
      continue;
    }

    // Emnpty path..
    if(path.IsEmpty()) continue;

    String include_path;
    if(search_local) {
      String search_path = Path::Canonical(Path::Combine(ps->gf->GetDirectory(),path));     
      if(File::Exists(search_path)) {
        include_path = search_path;
      }
    }

    if(include_path.IsEmpty()) {
      // Search the file's include directories
      for(auto it=ps->gf->GetIncludeDirectories().begin(); it!=ps->gf->GetIncludeDirectories().end(); it++) {
        String search_path = Path::Canonical(Path::Combine((*it),path));
        if(File::Exists(search_path)) {
          include_path = search_path;
          break;
        }
      }

      // Search the known include directories
      if(include_path.IsEmpty()) {
        for(auto it=ps->gs->GetIncludeDirectories().begin(); it!=ps->gs->GetIncludeDirectories().end(); it++) {
          String search_path = Path::Canonical(Path::Combine((*it),path));
          if(File::Exists(search_path)) {
            include_path = search_path;
            break;
          }
        }
      }
    }
      
    // If the include file could not found just silently ignore it.. This will
    // happen a lot with includes that are not within the project scope
    if(include_path.IsEmpty())  continue;

    // Add the file to the list of known files.
    GlueFile* inc_gf = ps->gs->GetFile(include_path);
    if(nullptr == inc_gf) {
      inc_gf = new GlueFile;
      inc_gf->SetFullPath (include_path);
      inc_gf->SetExcluded (true);

      // Copy the file level include directories to the new file.
      for(auto it=ps->gf->GetIncludeDirectories().begin(); it!=ps->gf->GetIncludeDirectories().end(); it++) {
        inc_gf->AddIncludeDirectory(*it);
      }

      if(!ps->gs->AddFile(inc_gf)) {
        return false;
      }
    }
  }

  return true;
}

bool GlueGen::ParseIncludedFiles (GlueState* gs) {
  // Loop over all files until all includes are parsed.  Two loops are used because
  // the internal ParseIncludes method can add additional files to be processed.
  noz_uint32 e = gs->GetFiles().size();
  for (noz_uint32 i=0; i<e; e=gs->GetFiles().size()) {
    for(; i<e; i++) {
      GlueFile* gf = gs->GetFiles()[i];
      noz_assert(gf);
      
      if(!gf->IsGlueFile()) continue;

      // Skip C files as they cant contain c++ definitions where we will find 
      // reflection data..
      if(gf->GetType() == GlueFileType::C || gf->GetType() == GlueFileType::Unknown) continue;

      ParseState ps;
      ps.gs = gs;
      ps.gf = gf;
      if(!ParseIncludedFiles(&ps)) return false;
    }
  }
  return true;
}


bool GlueGen::ParseDeclarations(ParseState* ps, GlueClass* nested_parent) {
  noz_assert(ps);
  noz_assert(ps->lexer);

  noz_int32 depth = 0;

  while(!ps->lexer->IsEnd()) {
    switch(ps->lexer->GetToken().type) {
      default:
        break;
    
      case GlueLexer::TokenType::OpenBrace:
        depth ++;
        break;

      case GlueLexer::TokenType::CloseBrace:
        ps->lexer->Skip();
        if(depth==0) return true;
        depth--;
        continue;
        
      case GlueLexer::TokenType::Namespace: {
        // Skip "namespace"
        ps->lexer->Skip();

        // Is this a legitimate namespace?
        Name name;
        if(ps->lexer->ExpectIdentifier(&name) && ps->lexer->Expect(GlueLexer::TokenType::OpenBrace) ) {
          ps->PushNamespace(name);
          if(!ParseDeclarations(ps)) return false;
          ps->PopNamespace();
        }
        continue;
      }

      case GlueLexer::TokenType::Enum: {
        ps->lexer->Skip();
        
        Name name;
        if(ps->lexer->Expect(GlueLexer::TokenType::Class) && ps->lexer->ExpectIdentifier(&name)) {
          GlueEnum* ge = ps->gs->AddEnum(ps->GetNamespace(), name);
          noz_assert(ge);
          ge->SetFile(ps->gf,ps->lexer->GetToken().pos->line);
        }
        continue;
      }

      case GlueLexer::TokenType::Struct:
      case GlueLexer::TokenType::Class: {
        // Skip Struct/Class
        ps->lexer->Skip();

        // Get Class name
        Name name = ParseQualifiedName(ps);
        
        // Forward declaration, skip
        if(ps->lexer->Expect(GlueLexer::TokenType::Semicolon)) continue;

        // Skip to open brace
        for(;!ps->lexer->IsEnd() && !ps->lexer->Expect(GlueLexer::TokenType::OpenBrace); ps->lexer->Skip());        

        if(name.IsEmpty()) {
          // Skip to close brace
          for(;!ps->lexer->IsEnd() && !ps->lexer->Expect(GlueLexer::TokenType::CloseBrace); ps->lexer->Skip());        
          continue;
        }

        // Add a new class..
        GlueClass* gc = ps->gs->AddClass(ps->GetNamespace(), name);
        noz_assert(gc);
        gc->SetFile(ps->gf,ps->lexer->GetToken().pos->line);
        gc->SetNestedParent(nested_parent);

        // Add the class as a namespace
        ps->PushNamespace(gc->GetName());

        // Recurse to handle bracing.
        if(!ParseDeclarations(ps, gc)) {
          return false;
        }

        ps->PopNamespace();

        continue;
      }
    }

    ps->lexer->Skip();
  }

  return true;
}

bool GlueGen::ParseDeclarations(GlueState* gs) {
  // Add std::list    
  GlueClass* classList = new GlueClass;
  classList->SetName("list");
  classList->SetQualifiedName("std::list");
  gs->AddClass(classList);

  // Add std::vector
  GlueClass* classVector = new GlueClass;
  classVector->SetName("vector");
  classVector->SetQualifiedName("std::vector");
  gs->AddClass(classVector);

  // Parse declarations from all known files.
  for(auto it=gs->GetFiles().begin(); it!=gs->GetFiles().end(); it++) {
    GlueFile* gf = *it;
    noz_assert(gf);

    if(!gf->IsGlueFile()) continue;
    if(gf->GetType() != GlueFileType::H) continue;

    ParseState ps;
    ps.gf = gf;
    ps.gs = gs;
    ps.lexer = gf->CreateLexer();

    if(!ParseDeclarations(&ps)) {
      return false;
    }
  }

  return true;
}

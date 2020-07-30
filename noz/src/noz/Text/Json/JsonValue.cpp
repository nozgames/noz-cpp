///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/StringReader.h>
#include <noz/IO/StreamReader.h>

#include "JsonValue.h"
#include "JsonObject.h"
#include "JsonArray.h"
#include "JsonString.h"

using namespace noz;

JsonValue::JsonValue (JsonType jt) {
  json_type_ = jt;
}

JsonValue* JsonValue::Load (TextReader& reader) {
  struct ParseHelper {
    static bool IsWhitespace (TextReader& reader) {
      char c = reader.Peek();
      return c=='\n' || c=='\r' || c==' ' || c == '\t';
    }

    static bool IsSeparator (TextReader& reader) {
      if(IsWhitespace(reader)) return true;
      char c = reader.Peek();
      return c==':' || c == ',' || c == '}' || c ==']';
    }

    static void SkipWhitespace (TextReader& reader) {
      for(;IsWhitespace(reader); reader.Read());
    }

    static bool ParseString (TextReader& reader, String& out) {
      StringBuilder sb;

      // Quoted string?
      if(reader.Peek()=='\"') {        
        for(reader.Read(); reader.Peek() && reader.Peek() != '\"'; reader.Read()) {
          if(reader.Peek()=='\\') {
            reader.Read();
            switch(reader.Peek()) {
              case '\\': sb.Append('\\'); break;
              case 'n': sb.Append('\n'); break;
              case 'f': sb.Append('\f'); break;
              case 'r': sb.Append('\r'); break;
              case 't': sb.Append('\t'); break;
              case 'b': sb.Append('\b'); break;
              case '/': sb.Append('/'); break;
              case '\"': sb.Append('\"'); break;
              default:
                break;
            }
            continue;
          }

          if(reader.Peek()=='\n') {
            return false;
          }
          sb.Append(reader.Peek());
        }
        out = sb.ToString();
        reader.Read();
        return true; 
      }

      while(reader.Peek() && !IsSeparator(reader)) {
        sb.Append((char)reader.Read());
      }
      out = sb.ToString();
      return true;
    }
  };

  // Root is alwasy an object
  JsonObject* root = new JsonObject;

  // Create value stack and add root object
  std::vector<JsonValue*> value_stack;
  value_stack.push_back(root);

  // Skip initial whitespace
  ParseHelper::SkipWhitespace(reader);

  // Object should start with a 
  if(reader.Read() != '{') return nullptr;

  // Skip over the opening '{' and any following whitespace
  ParseHelper::SkipWhitespace(reader);

  // Parse until the value stack is empty
  while(!value_stack.empty()) {
    // Comma?
    bool comma_found = false;
    if(reader.Peek() == ',') {
      comma_found= true;
      if(value_stack.back()->IsEmpty()) goto JsonValue_Parse_Error;

      // Skip comma
      reader.Read();
      ParseHelper::SkipWhitespace(reader);
    } 

    // End of Array?
    if(reader.Peek()==']') {
      if(!value_stack.back()->IsArray()) goto JsonValue_Parse_Error;
      reader.Read();
      ParseHelper::SkipWhitespace(reader);
      value_stack.pop_back();
      continue;
    }

    // End of Object?
    if(reader.Peek()=='}') {
      if(!value_stack.back()->IsObject()) goto JsonValue_Parse_Error;
      reader.Read();
      ParseHelper::SkipWhitespace(reader);
      value_stack.pop_back();
      continue;
    }

    // No comma and the current object has values..
    if(!comma_found && !value_stack.back()->IsEmpty()) goto JsonValue_Parse_Error;


    // Object value?
    if(value_stack.back()->IsObject()) {
      String key;
      ParseHelper::ParseString(reader,key);
      ParseHelper::SkipWhitespace(reader);
    
      // Key should alwasy be followed by a colon
      if(reader.Read() != ':') goto JsonValue_Parse_Error;

      // Skip the colon
      ParseHelper::SkipWhitespace(reader);

      // Handle all value types.
      switch (reader.Peek()) {
        case '[': {
          reader.Read();
          JsonArray* array = new JsonArray;
          ((JsonObject*)value_stack.back())->Add(key,array);
          value_stack.push_back(array);
          break;
        }

        case '{': {
          reader.Read();
          JsonObject* object = new JsonObject;
          ((JsonObject*)value_stack.back())->Add(key,object);
          value_stack.push_back(object);
          break;
        }

        default: {
          String value;
          ParseHelper::ParseString(reader,value);
          JsonString* str = new JsonString;
          str->Set(value);
          ((JsonObject*)value_stack.back())->Add(key,str);
          break;
        }
      }

      ParseHelper::SkipWhitespace(reader);
      continue;
    }

    // JsonArray ?
    if(value_stack.back()->IsArray()) {
      switch(reader.Peek()) {
        case '{': {        
          reader.Read();
          JsonObject* object = new JsonObject;
          ((JsonArray*)value_stack.back())->Add(object);
          value_stack.push_back(object);
          break;
        }

        case '[':  {
          reader.Read();
          JsonArray* array = new JsonArray;
          ((JsonArray*)value_stack.back())->Add(array);
          value_stack.push_back(array);
          break;
        }

        default: {
          String value;
          ParseHelper::ParseString(reader,value);
          JsonString* str = new JsonString;
          str->Set(value);
          ((JsonArray*)value_stack.back())->Add(str);
          break;
        }
      }

       ParseHelper::SkipWhitespace(reader);
      continue;
    }
  }

  return root;

JsonValue_Parse_Error:

  delete root;

  return nullptr;
  return nullptr;
}

JsonValue* JsonValue::Parse (const char* p) {
  struct ParseHelper {
    static bool IsWhitespace (const char* p) {
      return *p=='\n' || *p=='\r' || *p==' ' || *p == '\t';
    }

    static bool IsSeparator (const char* p) {
      if(IsWhitespace(p)) return true;
      return *p==':' || *p == ',' || *p == '}' || *p ==']';
    }

    static const char* SkipWhitespace (const char* p) {
      for(;IsWhitespace(p); p++);
      return p;
    }

    static const char* ParseString (const char* p, String& out) {
      // Quoted string?
      if(*p=='\"') {
        const char* s = p + 1;
        for(p++;*p && *p != '\"';p++) {
          if(*p=='\n') {
            break;
          }
        }
        out = String(s,0,p-s);
        return p+1;
      }

      const char* s = p;
      while(*p && !IsSeparator(p)) p++;
      out = String(s,0,p-s);
      return p;
    }
  };

  // Root is alwasy an object
  JsonObject* root = new JsonObject;

  // Create value stack and add root object
  std::vector<JsonValue*> value_stack;
  value_stack.push_back(root);

  // Skip initial whitespace
  p = ParseHelper::SkipWhitespace(p);

  // Object should start with a 
  if(*p != '{') return nullptr;

  // Skip over the opening '{' and any following whitespace
  p = ParseHelper::SkipWhitespace(p+1);

  // Parse until the value stack is empty
  while(!value_stack.empty()) {
    // Comma?
    bool comma_found = false;
    if(*p == ',') {
      comma_found= true;
      if(value_stack.back()->IsEmpty()) goto JsonValue_Parse_Error;

      // Skip comma
      p = ParseHelper::SkipWhitespace(p+1);
    } 

    // End of Array?
    if(*p==']') {
      if(!value_stack.back()->IsArray()) goto JsonValue_Parse_Error;
      p = ParseHelper::SkipWhitespace(p+1);
      value_stack.pop_back();
      continue;
    }

    // End of Object?
    if(*p=='}') {
      if(!value_stack.back()->IsObject()) goto JsonValue_Parse_Error;
      p = ParseHelper::SkipWhitespace(p+1);
      value_stack.pop_back();
      continue;
    }

    // No comma and the current object has values..
    if(!comma_found && !value_stack.back()->IsEmpty()) goto JsonValue_Parse_Error;


    // Object value?
    if(value_stack.back()->IsObject()) {
      String key;
      p = ParseHelper::SkipWhitespace(ParseHelper::ParseString(p,key));
    
      // Key should alwasy be followed by a colon
      if(*p != ':') goto JsonValue_Parse_Error;

      // Skip the colon
      p = ParseHelper::SkipWhitespace(p+1);

      // Handle all value types.
      switch (*p) {
        case '[': {
          JsonArray* array = new JsonArray;
          ((JsonObject*)value_stack.back())->Add(key,array);
          value_stack.push_back(array);
          break;
        }

        case '{': {
          JsonObject* object = new JsonObject;
          ((JsonObject*)value_stack.back())->Add(key,object);
          value_stack.push_back(object);
          break;
        }

        default: {
          String value;
          p = ParseHelper::SkipWhitespace(ParseHelper::ParseString(p,value));
          JsonString* str = new JsonString;
          str->Set(value);
          ((JsonObject*)value_stack.back())->Add(key,str);
          continue;
        }
      }

      p = ParseHelper::SkipWhitespace(p+1);
      continue;
    }

    // JsonArray ?
    if(value_stack.back()->IsArray()) {
      switch(*p) {
        case '{': {        
          JsonObject* object = new JsonObject;
          ((JsonArray*)value_stack.back())->Add(object);
          value_stack.push_back(object);
          break;
        }

        case '[':  {
          JsonArray* array = new JsonArray;
          ((JsonArray*)value_stack.back())->Add(array);
          value_stack.push_back(array);
          break;
        }

        default: {
          String value;
          p = ParseHelper::SkipWhitespace(ParseHelper::ParseString(p,value));
          JsonString* str = new JsonString;
          str->Set(value);
          ((JsonArray*)value_stack.back())->Add(str);
          continue;
        }
      }

      p = ParseHelper::SkipWhitespace(p+1);
      continue;
    }
  }

  return root;

JsonValue_Parse_Error:

  delete root;

  return nullptr;
}

JsonValue* JsonValue::Parse (const String& s) {
  return nullptr;
}

void JsonValue::ToString (const JsonValue* v, int depth, StringBuilder& out) {
  // JsonObject
  if(v->IsObject()) {
    if(((JsonObject*)v)->GetValues().empty()) {
      out.Append("{}\n");
    } else {
      out.Append("{\n");
      for(auto it=((JsonObject*)v)->GetValues().begin(); it!=((JsonObject*)v)->GetValues().end(); it++) {
        if(it!=((JsonObject*)v)->GetValues().begin()) out.Append(",\n");
        out.Append('\t', depth+1);
        out.Append('\"');
        out.Append(it->first);
        out.Append("\": ");
        ToString (it->second, depth+1, out);
      }
      out.Append('\n');
      out.Append('\t', depth);
      out.Append('}');
    }

  // JsonString
  } else if(v->IsString()) {
    out.Append('\"');
    const String& s = ((JsonString*)v)->Get();
    for(size_t i=0;i<(size_t)s.GetLength();i++) {
      switch(s[i]) {
        case '\\': out.Append("\\\\"); continue;
        case '\"': out.Append("\\\""); continue;
        case '/': out.Append("\\/"); continue;
        case '\b': out.Append("\\b"); continue;
        case '\f': out.Append("\\f"); continue;
        case '\n': out.Append("\\n"); continue;
        case '\r': out.Append("\\r"); continue;
        case '\t': out.Append("\\t"); continue;
        default: 
          out.Append(s[i]);
          break;
      }      
    }
    out.Append('\"');

  // JsonArray
  } else if(v->IsArray()) {
    if(((JsonArray*)v)->GetValues().empty()) {
      out.Append("[]\n");
    } else {
      out.Append("[\n");
      bool first=true;
      for(auto it=((JsonArray*)v)->GetValues().begin(); it!=((JsonArray*)v)->GetValues().end(); it++) {
        if(it!=((JsonArray*)v)->GetValues().begin()) out.Append(",\n"); 
        out.Append('\t', depth+1);
        ToString(*it,depth+1,out);
      }
      out.Append('\n');
      out.Append('\t',depth);
      out.Append(']');
    }
  }
}

String JsonValue::ToString (void) const {
  StringBuilder result;
  ToString(this,0,result);
  return result.ToString();
}


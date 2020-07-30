///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/TextReader.h>
#include "JsonReader.h"

using namespace noz;

JsonReader::JsonReader(TextReader* reader, const char* filename) {
  if(nullptr==filename) filename_ = "<unknown>"; else filename_ = filename;  
  reader_ = reader;
  state_.expect_ = 0;
  state_.line_ = 0;
  state_.column_ = 0;
  state_.token_line_ = 0;
  state_.token_column_ = 0;
  state_.position_ = reader->GetPosition();
  state_.stack_.push_back(NestType::Root);
  error_ = false;
  ReadToken();
}

bool JsonReader::ReadStartArray(void) {  
  if(!Is(TokenType::ArrayStart)) {
    ReportError("Array expected.");
    return false;
  }
  ReadToken();
  return true;
}

bool JsonReader::ReadStartObject(void) {
  if(!Is(TokenType::ObjectStart)) {
    ReportError("Object expected.");
    return false;
  }
  ReadToken();
  return true;
}

bool JsonReader::ReadEndArray(void) {
  if(!Is(TokenType::ArrayEnd)) {
    ReportError("The array is unclosed, ']' expected.");
    return false;
  }
  ReadToken();
  return true;
}

bool JsonReader::ReadEndObject(void) {
  if(!Is(TokenType::ObjectEnd)) {
    ReportError("The object is unclosed, '}' expected.");
    return false;
  }
  ReadToken();
  return true;
}

bool JsonReader::ReadMember(Name& value) {
  if(!Is(TokenType::Member)) {
    ReportError("Object member name expected.");
    return false;
  }
  value = state_.token_;
  ReadToken();
  return true;
}

bool JsonReader::ReadValueInt32(noz_int32& value) {
  if(!Is(TokenType::ValueNumber)) return false;
  value = Int32::Parse(state_.token_);
  ReadToken();
  return true;
}

bool JsonReader::ReadValueBoolean(bool& value) {
  if(!Is(TokenType::ValueBoolean)) return false;
  value = Boolean::Parse(state_.token_);
  ReadToken();
  return true;
}

bool JsonReader::ReadValueNull(void) {
  if(!Is(TokenType::ValueNull)) return false;
  ReadToken();
  return true;
}

bool JsonReader::ReadValueByte(noz_byte& value) {
  if(!Is(TokenType::ValueNumber)) return false;
  value = Byte::Parse(state_.token_);
  ReadToken();
  return true;
}

bool JsonReader::ReadValueUInt32(noz_uint32& value) {
  if(!Is(TokenType::ValueNumber)) return false;
  value = UInt32::Parse(state_.token_);
  ReadToken();
  return true;
}

bool JsonReader::ReadValueFloat(noz_float& value) {
  if(Is(TokenType::ValueString)) {
    value = Float::Parse(state_.token_);
  } else if(!Is(TokenType::ValueNumber)) {
    return false;
  } else {
    value = Float::Parse(state_.token_);
  }

  ReadToken();
  return true;
}

bool JsonReader::ReadValueString (String& out) {
  if(!Is(TokenType::ValueString)) return false;
  out = state_.token_;
  ReadToken();
  return true;
}

char JsonReader::Peek(void) {
  return reader_->Peek();
}

char JsonReader::Read(void) {
  char c = reader_->Read();
  state_.position_++;
  if(c==0) return c;
  if(c=='\n') {state_.line_++;state_.column_=0;return c;}
  if(c=='\\') {
    c = reader_->Read();
    state_.position_++;
    state_.column_++;
    switch(c) {
      case 'n': c = '\n'; break;
      case 'f': c = '\f'; break;
      case 'r': c = '\r'; break;
      case 't': c = '\t'; break;
      case '\\': c = '\\'; break;
      case '/': c = '/'; break;
      default:
        ReportInvalidEscapeSequence(c);
        break;
    }
  }
      
  state_.column_++;
  return c;
}

void JsonReader::SkipWhitespace (void) {
  for(char c=Peek(); c=='\t' || c==' ' || c=='\n' || c=='\r'; Read(),c=Peek());
}

void JsonReader::ReadToken(void) {  
  // Skip whitespace
  SkipWhitespace();  

  // End of document?
  char c = Peek();
  if(c==0) {
    state_.token_type_ = TokenType::End;  
    return;
  }

  NestType nest = state_.stack_.back();
  state_.token_ = String::Empty;

  // Check for comma between array elements and object members
  if(state_.expect_==',') {
    state_.expect_ = 0;
    if((nest==NestType::Array&&c!=']') || (nest==NestType::Object&&c!='}')) {
      // Expect a comma or end of array/object
      if(c!=',') {
        ReportMissingComma(); 
        return;
      }

      // Read the comma
      Read();
      // Skip whitespace
      SkipWhitespace();

      // Peek again.
      c = Peek();
    }    
  }

  // Expect a colon?
  bool member_value = false;
  if(state_.expect_ == ':') {
    state_.expect_ = 0;
    if(c!=':') {
      ReportMissingColon();
      return;
    }
    // Read the colon
    Read();
    // Skip Whitespace after colon
    SkipWhitespace();
    // Peek next char
    c = Peek();
    member_value = true;
  }

  state_.token_line_ = state_.line_;
  state_.token_column_ = state_.column_;

  switch(c) {
    // Start object
    case '{': {
      // Objects can only be at the root or as member values.
      if(nest != NestType::Root && !member_value && nest!=NestType::Array) {
        ReportUnexpectedCharacterSequence();
        return;
      }
      Read();
      state_.token_type_ = TokenType::ObjectStart;
      state_.token_ = String::Empty;
      state_.stack_.push_back(NestType::Object);
      return;
    }

    // End object
    case '}': {
      // Must be within an object
      if(nest!=NestType::Object) {
        ReportUnexpectedCharacterSequence();
        return;
      }
      Read();
      state_.stack_.pop_back();
      state_.token_type_ = TokenType::ObjectEnd;
      state_.token_ = String::Empty;
      state_.expect_ = ',';
      return;
    }

    // Start array
    case '[': {
      // Objects can only be at the root or as member values.
      if(nest != NestType::Root && !member_value && nest!=NestType::Array) {
        ReportUnexpectedCharacterSequence();
        return;
      }
      Read();
      state_.token_type_ = TokenType::ArrayStart;
      state_.token_ = String::Empty;
      state_.stack_.push_back(NestType::Array);
      return;
    }

    // End Array
    case ']': {
      // Must be within an object
      if(nest!=NestType::Array) {
        ReportUnexpectedCharacterSequence();
        return;
      }
      Read();
      state_.stack_.pop_back();
      state_.token_type_ = TokenType::ArrayEnd;
      state_.token_ = String::Empty;
      state_.expect_ = ',';
      return;
    }

    // Quoted string
    case '\"': {
      // Quoted string can be member name or array element or member value
      if(nest != NestType::Object && 
         nest != NestType::Array &&
         !member_value ) {
        ReportUnexpectedCharacterSequence();
        return;
      }

      // Read open quote
      Read();

      sb_.Clear();
      // Read until EOL or q quote is found.
      for(c=Peek(); c && c!='\"' && c!='\n'; c=Peek()) sb_.Append(Read());
      if(c!='\"') {
        ReportMissingClosingQuote();        
        return;
      }
      Read();     
      state_.token_ = sb_.ToString();

      // If nested in a member then the string is a member value.
      if(member_value || nest==NestType::Array) {
        state_.token_type_ = TokenType::ValueString;
        state_.expect_ = ',';

      // If within and object then the string is a member name
      } else if(nest == NestType::Object) {
        state_.token_type_ = TokenType::Member;
        state_.expect_ = ':';
      }

      break;
    }

    /// Number
    case '-':
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
      // Number is only supported a member value or array element
      if(!member_value && nest!=NestType::Array) {
        ReportUnexpectedCharacterSequence();
        return;
      }

      sb_.Clear();
      // Optional minus sign
      if(c == '-') {sb_.Append(c); c=ReadPeek();}
      
      // Must be followed by a digit
      if(!(c>='0' && c<='9')) {
        ReportUnexpectedCharacterSequence();
        return;
      }

      // Read as many numbers as available
      for(;c>='0' && c<='9'; c=ReadPeek()) sb_.Append(c);

      // Optional fraction
      if(c=='.') {
        // Add decimal point
        sb_.Append(c);
        Read();
        c = Peek();

        // Ensure decimal is followed by a digit
        if(!(c>='0' && c<='9')) {
          ReportUnexpectedCharacterSequence();
          return;
        }

        // Read numbers following decimal
        for(; c>='0' && c<='9'; c=ReadPeek()) sb_.Append(c);
      }
      // Optional exponent
      if(c=='e' || c=='E') {
        // Add the exponent indicator
        sb_.Append(c);
        Read();
        c=Peek();

        // Optional plus or minus
        if(c=='+' || c=='-') {
          sb_.Append(c);
          c=ReadPeek();
        }

        // Must be followed by a digit
        if(!(c>='0' && c<='9')) {
          ReportUnexpectedCharacterSequence();
          return;
        }

        // Read exponent digits
        for(; c>='0' && c<='9'; c=ReadPeek()) sb_.Append(c);
      }

      state_.token_type_ = TokenType::ValueNumber;
      state_.token_ = sb_.ToString();
      state_.expect_ = ',';
            
      break;
    }

    case 'N':
      if(ReadPeek()!='a') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='N') {ReportUnexpectedCharacterSequence(); return;}
      Read();
      state_.token_type_ = TokenType::ValueNumber;
      state_.token_ = sb_.ToString();
      state_.expect_ = ',';
      break;

    case 'I':
      if(ReadPeek()!='n') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='i') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='n') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='i') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='t') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='y') {ReportUnexpectedCharacterSequence(); return;}
      Read();
      state_.token_type_ = TokenType::ValueNumber;
      state_.token_ = sb_.ToString();
      state_.expect_ = ',';
      break;

    case 'n':
      if(ReadPeek()!='u') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='l') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='l') {ReportUnexpectedCharacterSequence(); return;}
      Read();
      state_.token_type_ = TokenType::ValueNull;
      state_.token_ = String::Empty;
      state_.expect_ = ',';
      break;

    case 'f':
      if(ReadPeek()!='a') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='l') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='s') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='e') {ReportUnexpectedCharacterSequence(); return;}
      Read();
      state_.token_type_ = TokenType::ValueBoolean;
      state_.token_ = "false";
      state_.expect_ = ',';
      break;

    case 't':
      if(ReadPeek()!='r') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='u') {ReportUnexpectedCharacterSequence(); return;}
      if(ReadPeek()!='e') {ReportUnexpectedCharacterSequence(); return;}
      Read();
      state_.token_type_ = TokenType::ValueBoolean;
      state_.token_ = "true";
      state_.expect_ = ',';
      break;

  }
}

bool JsonReader::SkipValue(void) {
  if(IsMember() || IsEOS()) {
    ReportError("Member value expected");
    return false;
  }

  switch(state_.token_type_) {
    case TokenType::ValueBoolean:
    case TokenType::ValueNull:
    case TokenType::ValueNumber:
    case TokenType::ValueString:
      ReadToken();
      return true;
  
    case TokenType::ObjectStart: {
      if(!ReadStartObject()) return false;
      while(!IsEndObject()) {
        Name name;
        if(!ReadMember(name)) return false;
        if(!SkipValue()) return false;
      }
      if(!ReadEndObject()) return false;
      return true;
    }

    case TokenType::ArrayStart: {
      if(!ReadStartArray()) return false;
      while(!IsEndArray()) {
        if(!SkipValue()) return false;
      }
      if(!ReadEndArray()) return false;
      return true;
    }

    default:
      break;
  }

  noz_assert(false);
  return false;
}

void JsonReader::ReportError(const char* msg, noz_uint32 line, noz_uint32 column) {
  state_.token_type_ = TokenType::Error;
  
  // Report only a single error
  if(error_) return;

  Console::WriteLine("%s(%d,%d): error: %s", filename_.ToCString(), line+1, column+1, msg);

  error_ = true;
}

void JsonReader::ReportWarning(const char* msg, noz_uint32 line, noz_uint32 column) {
  // Stop reporting warnings after an error has been reported
  if(error_) return;

  Console::WriteLine("%s(%d,%d): warning: %s", filename_.ToCString(), line+1, column+1, msg);
}

void JsonReader::ReportUnexpectedCharacterSequence(void) {
  switch(state_.stack_.back()) {
    case NestType::Object: ReportError("Unexpected character sequence in member.", state_.line_, state_.column_); break;
    case NestType::Array: ReportError("Unexpected character sequence in array element value.", state_.line_, state_.column_); break;
    default: ReportError("Unexpected character sequence.", state_.line_, state_.column_); break;
  }
}

void JsonReader::ReportMissingClosingQuote(void) {
  ReportError("Missing a closing quote for the string value", state_.line_, state_.column_);
}

void JsonReader::ReportInvalidEscapeSequence(char c) {
  ReportError(String::Format("Invalid escape sequence \"\\%c\"", c).ToCString(), state_.line_, state_.column_);
}

void JsonReader::ReportMissingComma(void) {
  switch(state_.stack_.back()) {
    case NestType::Array: ReportError("Missing a comma after an array element", state_.line_, state_.column_); break;
    case NestType::Object: ReportError("Missing a comma after an object member", state_.line_, state_.column_); break;
    default:
      noz_assert(false);
      break;     
  }
}

void JsonReader::ReportMissingColon(void) {  
  ReportError("Missing a colon (\":\") between the name and value int the \"(name) : (value)\" object member.", state_.line_, state_.column_);
}

noz_uint32 JsonReader::AddMarker(void) {
  markers_.push_back(new State(state_));
  return markers_.size()-1;
}

void JsonReader::SeekMarker (noz_uint32 marker) {
  state_ = *markers_[marker];
  reader_->SetPosition(state_.position_);
}

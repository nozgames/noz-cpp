///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/TextReader.h>
#include <stdlib.h>
#include "StringLexer.h"

using namespace noz;

StringLexer::StringLexer(TextReader* reader, String separators, noz_uint32 flags, String whitespace) {
  reader_ = reader;

  // Initialize the lookup table
  memset(lut_,(noz_byte)TokenType::Identifier,sizeof(lut_));
  lut_[0] = TokenType::End;
  for(noz_int32 i=0;i<separators.GetLength();i++) {
    lut_[separators[i]] = TokenType::Separator;
  }    
  for(noz_int32 i=0;i<whitespace.GetLength();i++) {
    lut_[whitespace[i]] = TokenType::Whitespace;
  }    
  if(flags & FlagLiteralNumber) {
    for(char n='0'; n<='9'; n++) lut_[n] = TokenType::LiteralNumber;
  }
  if(flags & FlagLiteralStrings) {
    lut_['\"'] = TokenType::LiteralString;
    lut_['\''] = TokenType::LiteralString;
  }

  line_ = 1;
  column_ = 1;
  token_.line_end = token_.line_start = line_;
  token_.column_end = token_.column_start = column_;
  peek_.line_end = peek_.line_start = line_;
  peek_.column_end = peek_.column_start = column_;

  SkipChar();
  column_ = 0;
  Read(&token_);
}

void StringLexer::AddSeperator(char sep) {
  lut_[sep] = TokenType::Separator;
}

void StringLexer::RemoveSeperator(char sep) {
  lut_[sep] = TokenType::Identifier;
}

void StringLexer::Read(void) {
  if(!peek_.value.IsEmpty()) {
    token_ = peek_;
    peek_.value = "";
  } else {
    Read(&token_);
  }
}

String StringLexer::Consume(void) {
  String value = token_.value;
  Read();
  return value;
}

bool StringLexer::Consume(TokenType tt) {
  if(token_.type==tt) {
    Read();
    return true;
  }
  return false;
}

bool StringLexer::Consume(char sep) {
  if(token_.type==TokenType::Separator && token_.value[0]==sep) {
    Read();
    return true;
  }
  return false;
}

String StringLexer::Peek(void) {
  if(!peek_.value.IsEmpty()) return peek_.value;
  Read(&peek_);
  return peek_.value;
}


void StringLexer::Read(Token* token) {
  // Advance the token positions.
  token->line_start = line_;
  token->column_start = column_;

  // Skip whitespace
  while(ct_ == TokenType::Whitespace) SkipChar();
  if(ct_ == TokenType::End) {
    token->type = TokenType::End;
    return;
  }

  sb_.Clear();



  switch(ct_) {
    case TokenType::Identifier:
      if(c_ == '.') {
        ConsumeChar();
        if(ct_ == TokenType::LiteralNumber) {
          ReadLiteralNumber(token,false);
          break;
        }
      }

      while(ct_ != TokenType::End && ct_ != TokenType::Separator && ct_ != TokenType::Whitespace) {
        ConsumeChar();
      }
      token->type = TokenType::Identifier;
      break;

    case TokenType::LiteralString: {
      char end = c_;
      SkipChar();
      while(!IsEnd() && c_ != end) {
        ConsumeChar();
      }
      SkipChar();
      token->type = TokenType::Identifier;
      break;
    }    

    case TokenType::LiteralNumber: {
      ReadLiteralNumber(token,true);
      break;
    }

    case TokenType::Separator:
      ConsumeChar();
      token->type = TokenType::Separator;
      break;
        
    default:
      noz_assert(false);
      break;
  }

  token->value = sb_.ToString();
  token->line_end = line_;
  token->column_end = column_;
}

void StringLexer::ReadLiteralNumber(Token* token, bool allow_decimal) {
  while(!IsEnd() && c_ >= '0' && c_ <='9') ConsumeChar();
  if(allow_decimal && c_=='.') {
    ConsumeChar();
    while(!IsEnd() && c_ >= '0' && c_ <='9') ConsumeChar();
  }
  token->type = TokenType::LiteralNumber;  
}

void StringLexer::SkipChar(void) {
  noz_int32 c = reader_->Read();
  if(c==-1) {
    c_ = 0;
  } else {
    c_ = (char)c;
  }

  if(c=='\n') {
    line_++;
    column_=1;
  } else {
    column_++;
  }
  
  ct_ = lut_[c_];
}

void StringLexer::ConsumeChar(void) {
  sb_.Append(c_);
  SkipChar();
}

bool StringLexer::ConsumeChar(TokenType t) {
  if(ct_ == t) {
    ConsumeChar();
    return true;
  }

  return false;
}

void StringLexer::ReportError (const char* format, ...) {
  va_list args;	
	va_start(args,format);
  va_list args2;
	va_start(args2,format);
  String out=String::Format(format,args,args2);
  va_end(args);
  va_end(args2);  

  Console::WriteLine("%s(%d,%d,%d,%d): %s", name_.ToCString(), token_.line_start, token_.column_start, token_.line_end, token_.column_end, out.ToCString());
}


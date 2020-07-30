///////////////////////////////////////////////////////////////////////////////
// noZ Glue
// Copyright (C) 2014 Bryan Dube / Radius Software
// http://www.radius-software.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "GlueGen.h"
#include "GlueLexer.h"

using namespace noz;
using namespace noz::Editor;

bool GlueLexer::lut_initialized = false;
bool GlueLexer::lut_whitespace[255] = {0};
bool GlueLexer::lut_eol[255] = {0};
bool GlueLexer::lut_identifier_start[255] = {0};
bool GlueLexer::lut_identifier_part[255] = {0};
bool GlueLexer::lut_number[255] = {0};
bool GlueLexer::lut_number_suffix_integer[255] = {0};
bool GlueLexer::lut_number_suffix_real[255] = {0};
GlueLexer::TokenType GlueLexer::lut_operator[255];
std::map<String,GlueLexer::TokenType> GlueLexer::lut_keywords;
std::map<GlueLexer::ComplexOperator,GlueLexer::TokenType> GlueLexer::lut_complex_operator;

GlueLexer::GlueLexer(const char* data, noz_uint32 size) {
  if(!lut_initialized) {
    lut_initialized = true;

    lut_whitespace['\n'] = true;
    lut_whitespace['\r'] = true;
    lut_whitespace['\t'] = true;
    lut_whitespace[' '] = true;

    for(int ii=0; ii<255; ii++) {
      lut_operator[ii] = TokenType::Unknown;
    }

    lut_operator['?'] = TokenType::Question;
    lut_operator['{'] = TokenType::OpenBrace;
    lut_operator['}'] = TokenType::CloseBrace;
    lut_operator['='] = TokenType::Assignment;
    lut_operator['/'] = TokenType::Divide;
    lut_operator['*'] = TokenType::Multiply;
    lut_operator['#'] = TokenType::Pound;
    lut_operator['['] = TokenType::OpenBracket;
    lut_operator[']'] = TokenType::CloseBracket;
    lut_operator[')'] = TokenType::CloseParen;
    lut_operator['('] = TokenType::OpenParen;
    lut_operator[':'] = TokenType::Colon;
    lut_operator[';'] = TokenType::Semicolon;
    lut_operator['.'] = TokenType::Dot;
    lut_operator[','] = TokenType::Comma;
    lut_operator['+'] = TokenType::Plus;
    lut_operator['-'] = TokenType::Minus;
    lut_operator['!'] = TokenType::Not;
    lut_operator['~'] = TokenType::Tilde;
    lut_operator['>'] = TokenType::GreaterThan;
    lut_operator['<'] = TokenType::LessThan;
    lut_operator['&'] = TokenType::LogicalAND;
    lut_operator['|'] = TokenType::LogicalOR;
    lut_operator['\''] = TokenType::SingleQuote;
    lut_operator['\\'] = TokenType::BackSlash;
    lut_operator['%'] = TokenType::Remainder;
    lut_operator['$'] = TokenType::Dollar;    

    // Scope operator..
    lut_complex_operator[ComplexOperator(TokenType::Colon,TokenType::Colon)] = TokenType::Scope;

    lut_eol['\r'] = true;
    lut_eol['\n'] = true;

    // Identifiers must start with a letter but then can contain any letter or number
    // as well as the underscore character
    for(unsigned char i='a'; i<='z'; i++) {lut_identifier_start[i]=lut_identifier_part[i]=true;}
    for(unsigned char j='A'; j<='Z'; j++) {lut_identifier_start[j]=lut_identifier_part[j]=true;}
    for(unsigned char k='0'; k<='9'; k++) {lut_identifier_part[k]=lut_number[k]=true;}
    lut_identifier_start['_'] = lut_identifier_part['_'] = true;

    lut_number_suffix_real['F'] = true;
    lut_number_suffix_real['f'] = true;
    lut_number_suffix_real['D'] = true;
    lut_number_suffix_real['d'] = true;
    lut_number_suffix_real['M'] = true;
    lut_number_suffix_real['m'] = true;
    lut_number_suffix_integer['U'] = true;
    lut_number_suffix_integer['u'] = true;
    lut_number_suffix_integer['L'] = true;
    lut_number_suffix_integer['l'] = true;

    lut_keywords["class"] = TokenType::Class;
    lut_keywords["const"] = TokenType::Const;
    lut_keywords["define"] = TokenType::Define;
    lut_keywords["enum"] = TokenType::Enum;
    lut_keywords["include"] = TokenType::Include;
    lut_keywords["namespace"] = TokenType::Namespace;
    lut_keywords["private"] = TokenType::Private;
    lut_keywords["protected"] = TokenType::Protected;
    lut_keywords["public"] = TokenType::Public;
    lut_keywords["static"] = TokenType::Static;
    lut_keywords["struct"] = TokenType::Struct;
    lut_keywords["template"] = TokenType::Template;
    lut_keywords["using"] = TokenType::Using;
    lut_keywords["virtual"] = TokenType::Virtual;
    lut_keywords["void"] = TokenType::Void;
    
    lut_keywords["NOZ_OBJECT"] = TokenType::NOZ_OBJECT;
    lut_keywords["NOZ_OBJECT_BASE"] = TokenType::NOZ_OBJECT_BASE;
    lut_keywords["NOZ_TEMPLATE"] = TokenType::NOZ_TEMPLATE;    
    lut_keywords["NOZ_ENUM"] = TokenType::NOZ_ENUM;
    lut_keywords["NOZ_INTERFACE"] = TokenType::NOZ_INTERFACE;
    lut_keywords["NOZ_METHOD"] = TokenType::NOZ_METHOD;
    lut_keywords["NOZ_PROPERTY"] = TokenType::NOZ_PROPERTY;
    lut_keywords["NOZ_CONTROL_PART"] = TokenType::NOZ_CONTROL_PART;
  }

  line_count_ = 0;

  // Look for UTF-8 encoding header
  if((unsigned char)(*data) == 0xEF && (unsigned char)(*(data+1))==0xBB && (unsigned char)(*(data+2))==0xBF) {
    data+=3;
    data_size_ -= 3;
  }

  data_ = data;
  data_size_ = size;  

  token_next_.type = TokenType::Unknown;

  current_ = data_;

  // Prime the lexer with the first two tokens to fill token_ and token_next
  ReadToken();
  ReadToken();
}

GlueLexer::~GlueLexer(void) {
}

const char* GlueLexer::SkipWhitespace(const char* data) {
  for(;lut_whitespace[*data];data++) {
    UpdateLine(data);
  }
  return data;
}

bool GlueLexer::ReadToken(void) {
  // Set the current token to the the value of the next token
  token_ = token_next_;

  // If the token is the end token then we are done
  if(token_.type == TokenType::End) return false;

  do {
    if(!ReadNextToken()) {
      return false;
    }
  } while (IsNext(TokenType::Comment) || IsNext(TokenType::BlockComment));
  
  return true;
}

bool GlueLexer::ReadNextToken(void) {
  // Skip whitespace
  if(!*(current_ = SkipWhitespace(current_))) {
    token_next_.type = TokenType::End;
    return true;
  }

  // Initialize the new token.
  token_next_.pos[0].ptr = token_next_.pos[1].ptr = current_;
  token_next_.pos[0].line = token_next_.pos[1].line = line_count_ + 1;
  token_next_.pos[0].column = token_next_.pos[1].column = current_ - current_line_;
  token_next_.type = TokenType::Unknown;  

  // Line comment..  We do this before the operators because a single slash is an operator
  if(*current_ == '/' && *(current_+1) == '/') {
    for(current_=current_+2;*current_ && !lut_eol[*current_];current_++) {
      UpdateLine(current_);
    }
    token_next_.type = TokenType::Comment;
    return true;

  // Block comment
  } else if(*current_ == '/' && *(current_+1) == '*') {
    for(current_=current_+2;*current_ && !(*(current_) == '*' && *(current_+1)=='/');current_++) UpdateLine(current_);
    if((*current_)=='*' && *(current_+1)=='/') current_+=2;
    token_next_.type=TokenType::BlockComment;
    return true;
  
  // Number
  } else if((*current_ == '.' && lut_number[*(current_+1)]) || lut_number[*current_]) {
    bool real = *current_ == '.';
    for(current_++;lut_number[*current_] || (!real && *current_=='.');current_++) {
      real = real | (*current_=='.');
    }

    // Handle exponents
    if(*current_ == 'E' || *current_ == 'e') {
      real = true;

      // Skip the 'e'
      current_++;

      // Should be + or - or a number
      if(*current_ == '+' || *current_ == '-') {
        current_++;
      } 

      for(current_++;lut_number[*current_];current_++);
    }

    if(!real && lut_number_suffix_integer[*current_]) {
      token_next_.type = TokenType::LiteralInteger;
      current_++;
      switch(*(current_-1)) {
        case 'u': case 'U':           
          if(*current_ == 'L' || *current_ == 'l') {
            current_++;
            token_next_.type = TokenType::LiteralUnsignedLong;
          } else {
            token_next_.type = TokenType::LiteralUnsignedInteger;
          }
          break;

        case 'l': case 'L':
          if(*current_ == 'u' || *current_ == 'U') {
            token_next_.type = TokenType::LiteralUnsignedLong;
            current_++;
          } else {
            token_next_.type = TokenType::LiteralLong;
          }
          break;
      }
    
    } else if (lut_number_suffix_real[*current_]) {
      current_++;
      token_next_.type = TokenType::LiteralReal;
    } else if (real) {
      token_next_.type = TokenType::LiteralReal;
    } else {
      token_next_.type = TokenType::LiteralInteger;
    }      

    token_next_.pos[1].line = line_count_ + 1;
    token_next_.pos[1].column = (current_-1) - current_line_;
    token_next_.pos[1].ptr = current_-1;

  // Operator  
  } else if(lut_operator[*current_] != TokenType::Unknown) {
    token_next_.type = lut_operator[*current_];
    current_++;

    TokenType op2 = lut_operator[*current_];

    auto it=lut_complex_operator.find(ComplexOperator(token_next_.type,op2));
    if(it!=lut_complex_operator.end()) {
      token_next_.type = it->second;
      current_++;
    }

    token_next_.pos[1].ptr = current_ - 1;
    token_next_.pos[1].line = line_count_ + 1;
    token_next_.pos[1].column = token_next_.pos[1].ptr - current_line_;

  // Quoted string
  } else if(*current_ == '\"') {
    token_next_.pos[0].ptr = current_+1;
    for(current_++;*current_ && *current_ != '\"'; current_++);
    if(*current_=='\"') {
      current_++;
    }
    token_next_.pos[1].ptr = current_-2;
    token_next_.pos[1].line = line_count_ + 1;
    token_next_.pos[1].column = token_next_.pos[1].ptr - current_line_;
    token_next_.type = TokenType::LiteralString;

  // Keyword and identifiers
  } else if(lut_identifier_start[*current_]) {
    for(current_++; *current_ && lut_identifier_part[*current_]; current_++);

    token_next_.pos[1].ptr = current_-1;
    token_next_.pos[1].line = line_count_ + 1;
    token_next_.pos[1].column = token_next_.pos[1].ptr - current_line_;
    
    auto it = lut_keywords.find(token_next_.ToString());
    if(it != lut_keywords.end()) {
      token_next_.type = it->second;
    } else {
      token_next_.type = TokenType::Identifier;
    }
  } else {
    token_next_.type = TokenType::Unknown;
    token_next_.pos[1].ptr = current_;
    token_next_.pos[1].line = line_count_ + 1;
    token_next_.pos[1].column = token_next_.pos[1].ptr - current_line_;    
    current_++;
  }
  
  return true;
}

void GlueLexer::Skip(void) {
  ReadToken();
}

bool GlueLexer::Expect(TokenType tt, const char* value) {
  if(token_.type!=tt || (value && token_.ToString().CompareTo(value))) {
    return false;
  }
  Skip();
  return true;
}

bool GlueLexer::ExpectIdentifier (void) {
  if(token_.type!=TokenType::Identifier) return false;
  Skip();
  return true;
}

bool GlueLexer::ExpectIdentifier (String* value) {
  // First make sure the token is an identifier.
  if(token_.type!=TokenType::Identifier) return false;

  // Optionally return its value.
  if(value) *value = token_.ToString();

  // Skip the token
  Skip();

  // Success!
  return true;
}

bool GlueLexer::ExpectIdentifier (Name* value) {
  // First make sure the token is an identifier.
  if(token_.type!=TokenType::Identifier) return false;

  // Optionally return its value.
  if(value) *value = token_.ToString();

  // Skip the token
  Skip();

  // Success!
  return true;
}

void GlueLexer::UpdateLine(const char* data) {
  if((*data=='\n' && *(data-1)!='\r') || *data == '\r') {
    line_count_ ++;
    current_line_ = data + 1;
  }
}

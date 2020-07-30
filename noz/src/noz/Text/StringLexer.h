///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_StringLexer_h__
#define __noz_StringLexer_h__

namespace noz {  

  class TextReader;

  class StringLexer {    
    private: TextReader* reader_;

    public: enum class TokenType : noz_byte {
      Identifier,
      End,
      Whitespace,
      Separator,
      LiteralNumber,
      LiteralString,
      Comment,
    };

    private: struct Token {
      String value;
      TokenType type;
      noz_int32 line_start;
      noz_int32 line_end;
      noz_int32 column_start;
      noz_int32 column_end;
    };

    public: static const noz_uint32 FlagNone           = 0;

    /// Indicates that the lexer should treat quoted strings as literal strings and remove the quotes
    public: static const noz_uint32 FlagLiteralStrings = NOZ_BIT(0);

    /// Indicates that the lexer should treat numbers as a single token, automatically parsing the 
    /// decimal place if given.
    public: static const noz_uint32 FlagLiteralNumber  = NOZ_BIT(1);

    /// Indicates the lexer should parse c-style comments 
    public: static const noz_uint32 FlagComments       = NOZ_BIT(2);


    /// Lookup table used to 
    private: TokenType lut_[256];

    /// String builder to reuse to build token strings
    private: StringBuilder sb_;

    private: char c_;

    private: TokenType ct_;

    /// Current token.
    private: Token token_;

    /// Next token if a peek was performed.
    private: Token peek_;    

    private: noz_int32 line_;
    private: noz_int32 column_;

    /// Optional name that can be assigned to the lexer for error reporting..
    private: String name_;

    /// Construct a new lexer
    public: StringLexer(TextReader* reader, String separators, noz_uint32 flags=FlagNone, String whitespace="\r\n\t ");

    /// Add an additional seperator to the lexer
    public: void AddSeperator(char sep);

    /// Remove a seperator from the lexer
    public: void RemoveSeperator(char sep);

    /// Set the optional name with the lexer for error reporting
    public: void SetName(String name) {name_ = name;}

    public: const String& GetToken(void) const {return token_.value;}

    /// Consume a single token from the stream
    public: String Consume (void);

    /// Consume the current token only if it matches the given token type
    public: bool Consume (TokenType tt);

    /// Consume the current token only if it matches the given separator type
    public: bool Consume (char sep);

    /// Returns true if the end of the stream has been reached
    public: bool IsEnd(void) const { return peek_.value.IsEmpty() && token_.type == TokenType::End; }

    /// Returns true if the current token is a separator token
    public: bool IsSeparator(void) const {return token_.type == TokenType::Separator;}

    /// Returns true if the current token is a separator token
    public: bool IsSeparator(char sep) const {return token_.type == TokenType::Separator && token_.value[0] == sep;}

    /// Returns true if the current token is an identifier
    public: bool IsIdentifier(void) const {return token_.type == TokenType::Identifier;}

    /// Returns true if the current token is a literal number
    public: bool IsLiteralNumber(void) const {return token_.type == TokenType::LiteralNumber; }

    /// Peek the next token
    public: String Peek(void);

    /// Get the line number of the current token
    public: noz_int32 GetLineNumber(void) const {return token_.line_start;}

    /// Get the column number of the current token
    public: noz_int32 GetColumn(void) const {return token_.column_start;}

    public: void ReportError (const char* format, ...);

    private: bool ConsumeChar(TokenType t);

    private: void ConsumeChar(void);

    private: void SkipChar(void);

    private: void Read(void);

    private: void Read(Token* token);

    private: void ReadLiteralNumber(Token* token, bool allow_decimal);

    private: void ReadLineComment (Token* token);

    private: void ReadBlockComment (Token* token);
  };

} // namespace noz


#endif //__noz_StringLexer_h__


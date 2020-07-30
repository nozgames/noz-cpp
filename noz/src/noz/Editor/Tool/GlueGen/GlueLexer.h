///////////////////////////////////////////////////////////////////////////////
// noZ Glue compiler
// Copyright (C) 2013-2014 Bryan Dube / Radius Software
// http://www.radius-software.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_GlueLexer_h__
#define __noz_Editor_GlueLexer_h__

#undef NOZ_METHOD
#undef NOZ_OBJECT
#undef NOZ_OBJECT_BASE
#undef NOZ_PROPERTY
#undef NOZ_CONTROL_PART
#undef NOZ_ENUM
#undef NOZ_INTERFACE
#undef NOZ_TEMPLATE

namespace noz {
namespace Editor {

  class GlueLexer {
    public: enum class TokenType {
      /// Unknown token
      Unknown,

      /// Keywords..
      KeywordBegin,
      Class,
      Const,
      Define,
      Enum,
      Include,
      Namespace,
      Private,
      Protected,
      Public,
      Static,
      Struct,
      Template,
      Using,
      Virtual,
      Void,
      KeywordEnd,
  
      // Operators
      OperatorBegin,
      Question,                 ///   ?
      Not,                      ///   !
      OpenParen,                ///   (
      CloseParen,               ///   )
      OpenBrace,                ///   {
      CloseBrace,               ///   }
      OpenBracket,              ///   [
      CloseBracket,             ///   ]
      LessThanOrEqualTo,        ///   <=
      LessThan,                 ///   <
      GreaterThanOrEqualTo,     ///   >=
      GreaterThan,              ///   >
      RightShift,               ///   >>
      LeftShift,                ///   <<
      Plus,                     ///   +
      Minus,                    ///   -
      Multiply,                 ///   *
      Divide,                   ///   /
      BackSlash,                ///   
      Dot,                      ///   .
      Comma,                    ///   ,
      Tilde,                    ///   ~
      Semicolon,                ///   ;
      Increment,                ///   ++
      Decrement,                ///   --
      Remainder,                ///   %
      LogicalAND,               ///   &
      LogicalOR,                ///   |
      LogicalXOR,               ///   ^
      ConditionalAnd,           ///   &&
      ConditionalOr,            ///   ||
      Assignment,               ///   =
      AdditionAssignment,       ///   +=
      SubtractionAssignment,    ///   -=
      MultiplicationAssignment, ///   *=
      DivisionAssignment,       ///   /=
      RemainderAssignment,      ///   %=
      ANDAssignment,            ///   &=
      ORAssignment,             ///   |=
      XORAssignment,            ///   ^=
      RightShiftAssignment,     ///   >>=
      LeftShiftAssignment,      ///   >>=
      Lamda,                    ///   =>
      NullCoalescing,           ///   ??
      Equality,                 ///   ==
      Inequality,               ///   !=
      SingleQuote,              ///   '
      Dollar,                   ///   $
      Scope,                    ///   ::
      Pound,                    ///   #
      Colon,                    ///   :
      OperatorEnd,

      /// Comments
      Comment,
      BlockComment,

      /// Identifier 
      Identifier,

      /// Literals
      LiteralBegin,
      LiteralBoolean,
      LiteralInteger,
      LiteralLong,
      LiteralUnsignedInteger,
      LiteralUnsignedLong,
      LiteralReal,
      LiteralCharacter,
      LiteralString,
      LiteralEnd,
    
      /// NOZ macros
      NOZ_OBJECT,
      NOZ_OBJECT_BASE,
      NOZ_TEMPLATE,
      NOZ_INTERFACE,
      NOZ_METHOD,
      NOZ_PROPERTY,
      NOZ_CONTROL_PART,
      NOZ_ENUM,
      
      /// Final token in stream
      End
    };

    public: struct Token {
      struct Position {
        Position(void) {}
        Position(unsigned int l, unsigned int c, const char* p) {line=l;column=c;ptr=p;}

        unsigned int line;
        unsigned int column;
        const char* ptr;
      };

      /// Begin (0) and end (0) positions within the token data
      Position pos[2];

      /// Type of token
      TokenType type;

      /// Create a string using the token c-string data
      String ToString(void) const {
        return pos[0].ptr==nullptr ?
          String::Empty :
          String(pos[0].ptr,0,pos[1].ptr-pos[0].ptr+1);
      }
    };

    private: static bool lut_initialized;
    private: static bool lut_whitespace[255]; 
    private: static TokenType lut_operator[255];
    private: static bool lut_eol[255];
    private: static bool lut_identifier_start[255];
    private: static bool lut_identifier_part[255];
    private: static bool lut_number[255];
    private: static bool lut_number_suffix_integer[255];
    private: static bool lut_number_suffix_real[255];
    private: static std::map<String,TokenType> lut_keywords;
    private: typedef std::pair<TokenType,TokenType> ComplexOperator;
    private: static std::map<ComplexOperator,TokenType> lut_complex_operator;

    private: const char* data_;

    private: noz_uint32 data_size_;

    private: const char* current_;

    private: Token token_;

    private: Token token_next_;
    
    /// Total number of lines found in the stream
    private: unsigned int line_count_;

    /// Start of current line
    private: const char* current_line_;

    /// Default constructor
    public: GlueLexer(const char* data, noz_uint32 size);

    /// Default destructor
    public: ~GlueLexer(void);

    /// Return the current token in the stream
    public: const Token& GetToken(void) const {return token_;}

    /// Return the next token in the stream
    public: const Token& GetNextToken (void) const {return token_next_;}

    /// Skip the current token
    public: void Skip (void);

    /// Check the current token for a match of the given token type and optionally
    /// of the given string value.  If both are a match the token is advanced to the 
    /// next token in the stream and true is returned.  If no match is found the token
    /// position remains unchanged and false will be returned.
    public: bool Expect(TokenType tt, const char* value=nullptr);

    public: bool ExpectIdentifier(void);

    /// Check if the current token is an identifier, if it is the current token
    /// will be advanced to the next token and the identifier value will optionally be
    /// returned.  If there is no match false will be returned and the current token
    /// will remain unchanged.
    public: bool ExpectIdentifier(String* value);

    public: bool ExpectIdentifier(Name* value);

    /// Returns true if the current token type matches the given token type
    public: bool Is (TokenType type) const {return token_.type == type;}

    public: static bool Is (const Token* token, TokenType type) {return token->type == type;}

    /// Returns true if the next token type matches the given token type
    public: bool IsNext (TokenType type) const {return token_.type!=TokenType::End && token_next_.type == type;}

    /// Returns true if the current token is an identifier and optionally checks the name of the
    /// the identifier.
    public: bool IsIdentifier (const char* name=nullptr) const {
      return token_.type == TokenType::Identifier && (name==nullptr || !token_.ToString().CompareTo(name));
    }

    /// Returns true if the curren token is a literal
    public: bool IsLiteral (void) const {
      return token_.type >= TokenType::LiteralBegin && token_.type < TokenType::LiteralEnd;
    }

    /// Returns true if the current token is a keyword.  Optionally a token type can be given to
    /// not only determine if the current token is a keyword but if it is the specific keyword given.  Note
    /// that if a token type is given that is not a keyword this method will always return false.
    public: bool IsKeyword(TokenType tt=TokenType::Unknown) const {      
      return IsKeyword(token_,tt);
    }

    public: static bool IsKeyword(const Token& token, TokenType tt=TokenType::Unknown) {
      return (token.type>TokenType::KeywordBegin&&token.type<TokenType::KeywordEnd) && (tt==TokenType::Unknown || tt==token.type);
    }

    /// Returns true if the current token is the end token
    public: bool IsEnd (void) const {return token_.type == TokenType::End;}


    private: const char* SkipWhitespace (const char* data);
    private: bool ReadToken (void);
    private: bool ReadNextToken (void);
    private: void UpdateLine (const char* data);
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_Lexer_h__

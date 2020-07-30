///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_JsonReader_h__
#define __noz_JsonReader_h__

namespace noz {

  class TextReader;

  class JsonReader {
    public: enum class TokenType : noz_byte {
      ObjectStart,
      ObjectEnd,
      ArrayStart,
      ArrayEnd,
      Member,
      ValueString,
      ValueNumber,
      ValueNull,
      ValueBoolean,
      Error,
      End
    };

    private: enum class NestType : noz_byte {
      Root,
      Object,
      Array
    };

    private: struct State {
      noz_uint32 line_;

      noz_uint32 column_;

      noz_uint32 position_;

      String token_;

      noz_uint32 token_line_;

      noz_uint32 token_column_;

      TokenType token_type_;

      std::vector<NestType> stack_;

      char expect_;
    };

    /// Current parse state
    private: State state_;

    // TODO: AddMarker will push state and return the index into the marker table.  
    private: std::vector<State*> markers_;

    private: bool error_;

    private: TextReader* reader_;

    private: StringBuilder sb_;

    private: String filename_;

    /// Create a json writer from the given text writer. Note the JsonReader
    /// will not take ownership of the writer.  The reader also takes an optional
    /// filename which is used to report errors.
    public: JsonReader (TextReader* reader, const char* filename=nullptr);

    public: bool IsEOS(void) const {return Is(TokenType::End) || Is(TokenType::Error);}
    public: bool IsStartArray(void) const {return Is(TokenType::ArrayStart);}
    public: bool IsStartObject(void) const {return Is(TokenType::ObjectStart);}
    public: bool IsEndArray(void) const {return Is(TokenType::ArrayEnd);}
    public: bool IsEndObject(void) const {return Is(TokenType::ObjectEnd);}
    public: bool IsValueNull(void) const {return Is(TokenType::ValueNull);}
    public: bool IsValueNumber(void) const {return Is(TokenType::ValueNumber);}
    public: bool IsValueString(void) const {return Is(TokenType::ValueString);}
    public: bool IsValueBoolean(void) const {return Is(TokenType::ValueBoolean);}
    public: bool IsMember(void) const {return Is(TokenType::Member);}

    public: bool ReadStartArray (void);
    public: bool ReadStartObject (void);
    public: bool ReadEndArray (void);
    public: bool ReadEndObject (void);
    public: bool ReadMember (Name& value);
    public: bool ReadValueString (String& value);
    public: bool ReadValueInt32 (noz_int32& value);
    public: bool ReadValueUInt32 (noz_uint32& value);
    public: bool ReadValueFloat (noz_float& value);
    public: bool ReadValueBoolean (bool& value);
    public: bool ReadValueByte (noz_byte& value);
    public: bool ReadValueNull (void);

    public: bool SkipValue (void);

    /// Add a new marker in the stream
    public: noz_uint32 AddMarker(void);

    /// Seek to the given marker previously added with AddMarker
    public: void SeekMarker (noz_uint32 marker);

    /// Return true if an error has occurred during parse
    public: bool HasError(void) const {return error_;}

    public: void ReportError(const char* msg, noz_uint32 line, noz_uint32 column);

    public: void ReportError(const char* msg) {ReportError(msg,state_.token_line_,state_.token_column_);}

    public: void ReportWarning (const char* msg, noz_uint32 line, noz_uint32 column);

    public: void ReportWarning(const char* msg) {ReportWarning(msg,state_.token_line_,state_.token_column_);}

    private: bool Is (TokenType tt) const {return state_.token_type_ == tt;}

    private: void ReadToken (void);
    private: char Peek (void);
    private: char Read (void);
    private: char ReadPeek (void) {Read();return Peek();}
    private: void SkipWhitespace (void);

    private: void ReportUnexpectedCharacterSequence(void);
    private: void ReportMissingClosingQuote(void);
    private: void ReportInvalidEscapeSequence(char c);
    private: void ReportMissingComma (void);
    private: void ReportMissingColon (void);
  };

} // namespace noz

#endif // __noz_JsonReader_h__

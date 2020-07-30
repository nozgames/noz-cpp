///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_TextReader_h__
#define __noz_IO_TextReader_h__

namespace noz {

  class TextReader : public Object {
    NOZ_OBJECT()

    /// Character read from stream for peek operation (zero if none)
    protected: char peek_;

    public: TextReader(void);

    /// Reads the next character from the text reader and advances the character position by one character.
    public: virtual noz_int32 Read(void);

    /// Reads the next character without changing the state of the reader or the character source. Returns 
    /// the next available character without actually reading it from the reader.
    public: noz_int32 Peek(void);
    
    /// Reads a specified maximum number of characters from the current reader and writes the data to a buffer, beginning at the specified index.
    public: virtual noz_int32 Read(char* buffer, noz_int32 index, noz_int32 count) {return -1;}

    /// Reads a line of characters from the text reader and returns the data as a string.
    public: String ReadLine(void);

    /// Reads all characters from the current position to the end of the stream.
    public: String ReadToEnd(void);

    public: virtual noz_uint32 GetPosition (void) const {noz_assert(false); return 0;}

    public: virtual void SetPosition (noz_uint32 pos) {noz_assert(false);}
  };

} // namespace noz


#endif //__noz_IO_TextReader_h__


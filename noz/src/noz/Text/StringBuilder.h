///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_StringBuilder_h__
#define __noz_StringBuilder_h__

namespace noz {

  class StringBuilder {
    private: std::vector<char> value_;

    public: StringBuilder(void);
    public: StringBuilder(const char* value);
    public: StringBuilder(String value);

    /// Appends the string representation of a specified Unicode character to this instance
    public: void Append(char value);
    public: void Append(char value, noz_int32 count);
    public: void Append(const char* value);
    public: void Append(const char* value, noz_int32 length);
    public: StringBuilder& Append(String value);
    
    public: StringBuilder& Insert(noz_int32 index, String value);
    public: StringBuilder& Insert(noz_int32 index, char value);

    public: void Clear(void);

    /// Converts the value of this instance to a String.
    public: String ToString (void) const;

    public: char operator[] (noz_int32 index) const {return value_[index];}

    public: noz_int32 GetLength(void) const {return noz_int32(value_.size());}
  };

} // namespace noz


#endif //__noz_StringBuilder_h__


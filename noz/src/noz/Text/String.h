///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_String_h__
#define __noz_String_h__

namespace noz {

  class Stream;

  enum class StringComparison {
    /// Compare strings using raw characters
    Ordinal,

    /// Compare strings using raw characters and ignore the case of the strings.
    OrdinalIgnoreCase
  };

  /**
   * Immutable string object..  By making the string immutable its contents can be
   * pooled to allow fast copying of strings.  Use StringBuilder to create a 
   * dynamic string.
   */
  class String {
    friend class StringBuilder;

    private: char* value_;
    private: noz_int32 length_;

    public: static String Empty;

    public: String(void);    
    public: String(const char* value);
    public: String(const char* value,noz_int32 start, noz_int32 length);
    public: String(const String& value);
    public: String(Stream* stream, noz_int32 length);
    public: ~String(void);

    public: String& operator= (const String& value);

    public: noz_int32 IndexOf (char value, noz_int32 start=0) const;
    public: noz_int32 IndexOf (const char* value, noz_int32 start=0, StringComparison comparison=StringComparison::Ordinal) const;
    public: noz_int32 IndexOf (const String& value, noz_int32 start=0, StringComparison comparison=StringComparison::Ordinal) const {return IndexOf(value.ToCString(),start,comparison);}

    public: noz_int32 LastIndexOf (char value) const;

    public: static String Format (const char* format, ...);
    public: static String Format (const char* format, va_list args, va_list args2);

    public: String Substring (noz_int32 startIndex) const;
    public: String Substring (noz_int32 startIndex,noz_int32 lenght) const;

    public: String Trim (void) const;
    public: String Lower (void) const;
    public: String Upper (void) const;

    public: bool IsEmpty(void) const {return length_==0;}

    public: noz_int32 GetLength(void) const {return length_;}

    public: static noz_int32 GetLength (const char* s);

    public: bool Equals (const char* s) const {return Equals(s,StringComparison::Ordinal);}
    public: bool Equals (const String& s) const {return Equals(s,StringComparison::Ordinal);}
    public: bool Equals (const char* s, StringComparison comparison) const {return 0==CompareTo(s,comparison);}
    public: bool Equals (const String& s, StringComparison comparison) const;

    public: noz_int32 CompareTo(const String& s) const {return CompareTo(s,StringComparison::Ordinal);}
    public: noz_int32 CompareTo(const char* s) const {return CompareTo(s,StringComparison::Ordinal);}
    public: noz_int32 CompareTo(const String& compare, StringComparison comparison) const;
    public: noz_int32 CompareTo(const char* s, StringComparison comparison) const;

    public: static noz_int32 Compare(const String& compare1, noz_int32 index1, const String& compare2, noz_int32 index2, noz_int32 length);

    public: bool StartsWith (const String& s) const {return StartsWith(s,StringComparison::Ordinal);}
    public: bool StartsWith (const String& s, StringComparison comparison) const;

    public: const char* ToCString(void) const {return value_;}

    public: bool operator< (const String& s) const {return CompareTo(s)<0;}
    public: bool operator> (const String& s) const {return CompareTo(s)>0;}

    public: char operator[] (noz_int32 index) const {return value_[index];}

    public: noz_uint32 GetHashCode (void) const {return GetHashCode(value_);}

    public: static noz_uint32 GetHashCode (const char* s) ;
  };

  inline bool String::Equals (const String& s, StringComparison comparison) const {
    if(s.GetLength() != GetLength()) return false;
    return 0==CompareTo(s,comparison);
  }

}

#endif // __noz_String_h__

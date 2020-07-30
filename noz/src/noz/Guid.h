///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Guid_h__
#define __noz_Guid_h__

namespace noz {

  class Guid {
    private: noz_uint64 value_l_;
    private: noz_uint64 value_h_;

    public: static Guid Empty;

    public: Guid(void);

    public: Guid(noz_uint64 h, noz_uint64 l);

    public: String ToString (void) const;

    public: noz_uint64 GetHighOrder (void) const {return value_h_;}
    public: noz_uint64 GetLowOrder (void) const {return value_l_;}

    /// Generate a new Guid
    public: static Guid Generate (void);

    public: bool IsEmpty(void) const {return value_h_ == 0 && value_l_ == 0;}

    public: bool operator == (const Guid& c) const {return value_h_ == c.value_h_ && value_l_ == c.value_l_;}
    public: bool operator != (const Guid& c) const {return value_h_ != c.value_h_ || value_l_ != c.value_l_;}
    public: bool operator < (const Guid& c) const {
      if(value_h_==c.value_h_) return value_l_ < c.value_l_;
      return value_h_ < c.value_h_;
    }

    public: static Guid Parse (const char* p);
    public: static Guid Parse (const String& p) {return Parse(p.ToCString());}
  };

} // namespace noz


#endif //__noz_Guid_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_DateTime_h__
#define __noz_DateTime_h__

namespace noz {

  class DateTime : public Object {
    NOZ_OBJECT()

    private: static const noz_uint64 TicksMask       = 0x3FFFFFFFFFFFFFFF;
    
    // Number of 100ns ticks per time unit
    public: static const noz_int64 TicksPerMillisecond = 10000;
    public: static const noz_int64 TicksPerSecond = TicksPerMillisecond * 1000;
    public: static const noz_int64 TicksPerMinute = TicksPerSecond * 60;
    public: static const noz_int64 TicksPerHour = TicksPerMinute * 60;
    public: static const noz_int64 TicksPerDay = TicksPerHour * 24;
    
    // Number of milliseconds per time unit
    private: static const noz_int32 MillisPerSecond = 1000;
    private: static const noz_int32 MillisPerMinute = MillisPerSecond * 60;
    private: static const noz_int32 MillisPerHour = MillisPerMinute * 60;
    private: static const noz_int32 MillisPerDay = MillisPerHour * 24;

    private: static const noz_int32 DaysPerYear = 365;

    /// Number of days in 4 years
    private: static const noz_int32 DaysPer4Years = DaysPerYear * 4 + 1;       // 1461

    /// Number of days in 100 years
    private: static const noz_int32 DaysPer100Years = DaysPer4Years * 25 - 1;  // 36524

    /// Number of days in 400 years
    public: static const noz_int32 DaysPer400Years = DaysPer100Years * 4 + 1; // 146097
       
    /// Number of days from 1/1/0001 to 12/30/1899
    private: static const noz_int32 DaysTo1899 = DaysPer400Years * 4 + DaysPer100Years * 3 - 367;
    
    /// Number of days from 1/1/0001 to 12/31/1969
    public: static const noz_int32 DaysTo1970 = DaysPer400Years * 4 + DaysPer100Years * 3 + DaysPer4Years * 17 + DaysPerYear; // 719,162
    
    /// Number of days from 1/1/0001 to 12/31/9999
    private: static const noz_int32 DaysTo10000 = DaysPer400Years * 25 - 366;  // 3652059
    
    private: static const noz_uint64 MinTicks = 0;
    private: static const noz_uint64 MaxTicks = DaysTo10000 * TicksPerDay - 1;
    private: static const noz_uint64 MaxMillis = (noz_uint64 )DaysTo10000 * MillisPerDay;
   
    public: static const DateTime MinValue;
    public: static const DateTime MaxValue;
 
    private: noz_uint64 data_;

    public: DateTime (noz_uint64 ticks);
    public: DateTime (void);

    public: bool IsEmpty (void) const {return data_ == 0;}

    public: noz_int32 CompareTo (const DateTime& dt) const;

    public: bool operator < (const DateTime& dt) const {return data_ < dt.data_;}
    public: bool operator > (const DateTime& dt) const {return data_ > dt.data_;}

    private: noz_uint64 GetTicks (void) const {return data_ & TicksMask;}
  };

} // namespace noz


#endif //__noz_DateTime_h__


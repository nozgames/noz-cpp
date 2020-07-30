///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Math_h__
#define __noz_Math_h__

#include "Byte.h"
#include "Int32.h"
#include "UInt32.h"
#include "Double.h"
#include "Float.h"
#include "Boolean.h"

namespace noz {

  class Vector2;
  class Rect;

  NOZ_ENUM() enum class Stretch {
    /// Maintain origianl size
    None,

    /// Stretch to fit destination size ignoring aspect ratio
    Fill,

    /// Stretch to fit destination size while maintaining aspect ratio
    Uniform,

    /// Stretch to fit destination size while maintaining aspect ratio and filling
    /// the entire destintation size, clipping may occur if aspect ratios dont match.
    UniformToFill
  };

  class Math {
    // Static class
    private: Math(void) {}

    /// Max
    //public: static noz_uint8 Max(noz_uint8 a, noz_uint8 b) {return a>b?a:b;}
    public: static noz_int32 Max(noz_int32 a, noz_int32 b) {return a>b?a:b;}
    public: static noz_uint32 Max(noz_uint32 a, noz_uint32 b) {return a>b?a:b;}
    public: static noz_float Max(noz_float a, noz_float b) {return a>b?a:b;}
    public: static noz_double Max(noz_double a, noz_double b) {return a>b?a:b;}
    public: static Vector2 Max(const Vector2& a, const Vector2& b);

    /// Min
    //public: static noz_uint8 Min(noz_uint8 a, noz_uint8 b) {return a<b?a:b;}
    public: static noz_int32 Min(noz_int32 a, noz_int32 b) {return a<b?a:b;}
    public: static noz_uint32 Min(noz_uint32 a, noz_uint32 b) {return a<b?a:b;}
    public: static noz_float Min(noz_float a, noz_float b) {return a<b?a:b;}
    public: static noz_double Min(noz_double a, noz_double b) {return a<b?a:b;}
    public: static Vector2 Min(const Vector2& a, const Vector2& b);
    
    /// Ceil
    public: static noz_float Ceil(noz_float v);

    /// Floor
    public: static noz_float Floor(noz_float v);

    // Clamp
    public: static noz_float Clamp(noz_float v, noz_float minv, noz_float maxv) {return Max(Min(v,maxv),minv);}
    public: static noz_int32 Clamp(noz_int32 v, noz_int32 minv, noz_int32 maxv) {return Max(Min(v,maxv),minv);}

    /// Abs
    public: static noz_float Abs(noz_float a) {return a<0?-a:a;}
    public: static noz_int32 Abs(noz_int32 a) {return a<0?-a:a;}

    // Sqrt
    static noz_float Sqrt (noz_float a);
    static noz_double Sqrt (noz_double a);

    // Sin
    static noz_float Sin (noz_float a);

    // Cos
    static noz_float Cos (noz_float a);

    // ATan2
    static noz_float ATan2 (noz_float y, noz_float x);

    // ATan
    static noz_float ATan (noz_float t);

    // Mod
    static noz_float Mod (noz_float n, noz_float d) {return n - d * ((noz_int32)(n/d));}

    // Deg2Rad
    inline static noz_float Deg2Rad (noz_float a) {return (a * Float::PI_180);}

    // Rad2Deg
    inline static noz_float Rad2Deg (noz_float r) {return (r * Float::M_180_PI);}

    static noz_int32 Random (noz_int32 min, noz_int32 max);

    /// Next power of 2
    inline static noz_uint32 NextPow2 (noz_uint32 value) {
      value = value - 1;
      value = value | (value >> 1);
      value = value | (value >> 2);
      value = value | (value >> 4);
      value = value | (value >> 8);
      value = value | (value >> 16);
      value = value + 1;
      return value;
    }

    inline static noz_int32 RoundUp8 (noz_int32 numToRound) { 
      return (numToRound + 8 - 1) & ~(8 - 1);
    }

    inline static noz_byte Hex2Dec (char hex) {
      static const noz_byte lut[120]={
        0, 0, 0, 0, 0,   0, 0, 0, 0, 0,   0, 0, 0, 0, 0,   0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,   0, 0, 0, 0, 0,   0, 0, 0, 0, 0,   0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,   0, 0, 0, 0, 1,   2, 3, 4, 5, 6,   7, 8, 9, 0, 0,
        0, 0, 0, 0, 0,  10,11,12,13,14,  15, 0, 0, 0, 0,   0, 0, 0, 0, 0,
        0, 0, 0, 0, 0,   0, 0, 0, 0, 0,   0, 0, 0, 0, 0,   0, 0,10,11,12,
       13,14,15, 0, 0,   0, 0, 0, 0, 0,   0, 0, 0, 0, 0,   0, 0, 0, 0, 0
      };
      return lut[noz_int32(hex)];
    }
    
    /// Returns true if a given number is prime
    static bool IsPrime (noz_int32 i) {return IsPrime((noz_uint32)i);}
    static bool IsPrime (noz_uint32 i);
    static bool IsPrime (noz_uint64 i);

    /// Returns the prime number at the given index within a table of
    /// prime numbers.  For example a value of 0 will return 1, a value
    /// of 1 will return 2, etc..  This method is useful for walking the 
    /// list of prime numbers.
    static noz_uint32 GetPrimeForIndex (noz_uint32 index);

    static noz_uint32 Hex2Dec (const String& s, noz_uint32* mask=nullptr) {return Hex2Dec(s.ToCString(),mask);}
    static noz_uint32 Hex2Dec (const char* s, noz_uint32* mask=nullptr);

    static noz_float EvaluateCurve (noz_float t, noz_float val1, noz_float tan1, noz_float val2, noz_float tan2);


    static noz_int32 CountTrailingZeroBits (noz_uint64 mask);
    static noz_int32 CountTrailingZeroBits (noz_uint32 mask);

    static noz_int32 CountLeadingZeroBits (noz_uint64 mask);
    static noz_int32 CountLeadingZeroBits (noz_uint32 mask);

    static Rect StretchRect (Stretch s, const Vector2& ss, const Rect& r);
  };

} // namespace noz

#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "Rect.h"

namespace noz {

  inline Vector2 Math::Max(const Vector2& a, const Vector2& b) {
    return Vector2(Math::Max(a.x,b.x), Math::Max(a.y,b.y));
  }

  inline Vector2 Math::Min(const Vector2& a, const Vector2& b) {
    return Vector2(Math::Min(a.x,b.x), Math::Min(a.y,b.y));
  }

}

#endif //__noz_Math_h__


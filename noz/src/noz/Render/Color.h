///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Color_h__
#define __noz_Color_h__

namespace noz {

  class Vector4;

  struct Color {
    union {
      struct {
        noz_byte r;
        noz_byte g;
        noz_byte b;
        noz_byte a;
      };
      noz_uint32 raw;
    };

    static const Color Black;
    static const Color Blue;
    static const Color Green;
    static const Color Red;
    static const Color White;

    public: Color(void) {r=255;g=255;b=255;a=255;}
    public: Color(noz_uint32 _raw) {raw=_raw;}
    public: Color(noz_int32 _r, noz_int32 _g, noz_int32 _b, noz_int32 _a) {r=(noz_byte)_r;g=(noz_byte)_g;b=(noz_byte)_b;a=(noz_byte)_a;}
    public: Color(noz_float _r, noz_float _g, noz_float _b, noz_float _a) {
      r = (noz_byte)(_r * 255.0f);
      g = (noz_byte)(_g * 255.0f);
      b = (noz_byte)(_b * 255.0f);
      a = (noz_byte)(_a * 255.0f);
    }
    public: Color(const Vector4& v);

    static Color Lerp(const Color& c1, const Color& c2, noz_float lerp) {
      float ilerp=1.0f-lerp;
      return Color(
        noz_byte(ilerp * c1.r + (lerp * c2.r)),
        noz_byte(ilerp * c1.g + (lerp * c2.g)),
        noz_byte(ilerp * c1.b + (lerp * c2.b)),
        noz_byte(ilerp * c1.a + (lerp * c2.a))
      );
    }

    Color Lerp(const Color& c2, noz_float lerp) {
      return Lerp(*this, c2, lerp);
    }

    Color Add (Color color) const {
      return Color (
        (noz_byte)(r + color.r),
        (noz_byte)(g + color.g),
        (noz_byte)(b + color.b),
        (noz_byte)(a + color.a)
      );
    }

    /// Multiply the the color by the given color
    Color Modulate (Color color) const {
      return Color(
        noz_byte((noz_uint32)((r * color.r)) >> 8),
        noz_byte((noz_uint32)((g * color.g)) >> 8),
        noz_byte((noz_uint32)((b * color.b)) >> 8),
        noz_byte((noz_uint32)((a * color.a)) >> 8)
      );
    }

    /// Multiply the alpha component of the color by the given alpha component
    Color ModulateAlpha (noz_byte alpha) const {
      return Color(r,g,b,(noz_byte)(((alpha*a)>>8)&0xFF));
    }

    /// Multiply the alpha component by the given float value
    Color ModulateAlpha (noz_float alpha) const {
      return Color(r,g,b,noz_byte(alpha*a));
    }

    /// Convert to a float form within a Vector4.
    Vector4 ToVector4(void) const;

    /// Comparison operators
    bool operator== (const Color& c) {return raw==c.raw;}
    bool operator!= (const Color& c) {return raw!=c.raw;}

    public: static Color Parse (const String& s) {return Parse(s.ToCString());}
    public: static Color Parse (const char* s);

    String ToString (void) const;
  };


  class ColorObject : public Object {
    NOZ_OBJECT(TypeCode=Color)

    protected: Color value_;

    public: ColorObject(const Color& value) {value_ = value;}

    public: virtual bool Equals (Object* o) const override;

    public: Color operator* (void) {return value_;}

    public: Color GetValue (void) const {return value_;}
  };


} // namespace noz


#endif // __noz_Graphics_Color_h__


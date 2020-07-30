///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Vector2_h__
#define __noz_Vector2_h__

namespace noz {

  class Vector3;

  class Vector2 {
    public: static Vector2 Empty;
    public: static Vector2 Zero;
    public: static Vector2 One;
    public: static Vector2 OneZero;
    public: static Vector2 ZeroOne;
    public: static Vector2 NaN;

	  public: float x;
	  public: float y;
 
    public: Vector2 (void) {
	    x = 0.0f; y = 0.0f;
	  }
	  
    public: Vector2 (noz_float v) {
      x = v; y = v;
    }

	  public: Vector2 (noz_float _x, noz_float _y) {
	    x = _x; y = _y;
	  }

    public: Vector2 (const Vector3& v);
	  
    public: const float& operator[] (int i) const {return (&x)[i];}
	  public: float& operator[] (int i) {return (&x)[i];}
	  
	  public: Vector2&	operator+= (const Vector2& v) {
	    x += v.x; y += v.y;
	    return *this;
	  }
	  
	  public: Vector2&	operator-= (const Vector2& v) {
	    x -= v.x; y -= v.y;
      return *this;
	  }
	  
	  public: Vector2&	operator*= (float v) {
	    x *= v; y *= v;
	    return *this;
	  }
	  
	  public: Vector2& operator/= (float v) {
	    v = 1.0f / v;
	    x *= v; y *= v;
	    return *this;
	  }
	  
	  public: Vector2 operator- (const Vector2& v) const {
	    return Vector2 (x - v.x, y - v.y);
	  }
	  
	  public: Vector2 operator+ (const Vector2& v) const {
	    return Vector2 (x + v.x, y + v.y);
	  }

	  public: float operator*	(const Vector2& v) const {
	    return x * v.x + y * v.y;
	  }
	  
	  public: Vector2 operator* (float v) const {
	    return Vector2 (x * v, y * v);
	  }
	  
	  public: Vector2 operator/ (float v) const {
	    v = 1.0f / v;
	    return Vector2 (x * v, y * v);
	  }
	  
	  public: Vector2 operator- (void) const { 
	    return Vector2 (-x,-y); 
	  }

    public: bool operator== (const Vector2& v) const {
      return x == v.x && y == v.y;
    }
    
	  public: bool operator!=	(const Vector2& v) const {
	    return !(x == v.x && y == v.y);
	  }

    public: void set (float _x, float _y) {
      x = _x; y = _y;
    }

	  public: float normalize	(void);

	  public: void clear (void) {
	    x = 0.0f; y = 0.0f;
	  }
	  
    public: float length (void) const {
      return Math::Sqrt (x*x + y*y);
    }
    
    public: float length_sqr (void) const {
      return x*x + y*y;
    }
    
    public: float min (void) const {return Math::Min(x,y);}
    
    public: float max (void) const {return Math::Max(x,y);}
        
    public: Vector2 perpendicular (void) const {
      return Vector2 ( y, -x );
    }

    public: Vector2 Normalized (void) const;
    
    public: Vector2 squared (void) const {
      return Vector2 (x * x, y * y);
    }
    
    public: Vector2& square (void) {
      x *= x; y *= y;
      return *this;
    }
      
    public: bool empty (void) const {
      return x == 0.0f && y == 0.0f;
    }

    public: float cross (const Vector2& i_other) {
      return (x*i_other.y) - (y*i_other.x);
    }

    public: static Vector2 Parse(const char* s);
    public: static Vector2 Parse(const String& s) {return Parse(s.ToCString());}
  };

  class Vector2Object : public Object {
     NOZ_OBJECT(TypeCode=Vector2)

    protected: Vector2 value_;

    public: Vector2Object(const Vector2& value) {value_ = value;}

    public: virtual bool Equals (Object* o) const override;

    public: const Vector2& operator* (void) {return value_;}

    public: const Vector2& GetValue (void) const {return value_;}
  };

} // namespace noz


#endif //__noz_Vector2_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Vector4_h__
#define __noz_Vector4_h__

namespace noz {

  class Vector4 {
	  public: union {
      noz_float x;
      noz_float l;
    };
	  public: union {
      noz_float y;
      noz_float t;
    };
	  public: union {
      noz_float z;
      noz_float r;
    };
	  public: union {
      noz_float w;
      noz_float b;
    };

	  public: Vector4 (void) {
	    x = y = z = w = 0.0f;
	  }
	  
	  public: Vector4 (float _x, float _y, float _z, float _w) {
	    x = _x; y = _y; z = _z; w = _w;
	  }
	  
	  public: Vector4 (const Vector4& v) {
	    x = v.x; y = v.y; z = v.z; w = v.w; 
	  }

    public: void Clear (void) {x=y=z=w=0.0f;}

	  public: const float& operator[] (int i) const {return (&x)[i];}
	  
	  public: float& operator[] (int i) {return (&x)[i];}	  

    public: Vector4 operator* (noz_float s) const {return Vector4(x*s,y*s,z*s,w*s);}

    public: Vector4& operator+= (const Vector4& v) {x+=v.x;y+=v.y;z+=v.z;w+=v.w;return *this;}
  };

} // namespace noz

#endif // __noz_Vector4_h__



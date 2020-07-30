///////////////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Matrix3_h__
#define __noz_Matrix3_h__

#include "Vector2.h"

namespace noz {

  class Rect;

  struct Matrix3 {
    public: float d[9];    
    public: static Matrix3 Identity;

    public: Matrix3 (void) {}
	  
	  public: Matrix3 ( 
	    float _00, float _01, float _02,
	    float _10, float _11, float _12,
	    float _20, float _21, float _22
	  ) {
	    d[0] = _00; d[1] = _01; d[2] = _02;
	    d[3] = _10; d[4] = _11; d[5] = _12;
	    d[6] = _20; d[7] = _21; d[8] = _22;
	  }
	  
	  public: Matrix3 (const Matrix3& m);

	  
    public: Matrix3& identity (void) {
	    d[1] = d[2] = d[3] = d[5] = d[6] = d[7] = 0.0f;
	    d[0] = d[4] = d[8] = 1.0f; 
      return *this;
	  }
	     
	  public: Matrix3 transposed (void) const {
	    return Matrix3 (d[0], d[3], d[6],
	                    d[1], d[4], d[7],
	                    d[2], d[5], d[8]);
	  }
	  
	  public: Matrix3& transpose (void) {
	    *this = transposed ( );
      return *this;
	  }

    /// Translate the matrix
    public: inline Matrix3& translate (noz_float x, noz_float y) {
      Matrix3 trans(1.0f,0.0f,0.0f,   0.0f,1.0f,0.0f,  x,y,1.0f);
      *this = *this * trans;  
      return *this;
    }

    public: inline Matrix3& scale (noz_float x, noz_float y) {
      Matrix3 scale(x,0.0f,0.0f, 0.0f,y,0.0f, 0.0f, 0.0f, 1.0f);
      *this = *this * scale;  
      return *this;
    }

    public: inline Matrix3& rotate (noz_float angle) {
      noz_float s = Math::Sin (angle);
      noz_float c = Math::Cos (angle);
      Matrix3 rot(c,-s,0.0f, s,c,0.0f, 0.0f,0.0f,1.0f);
      *this = *this * rot;
      return *this;
    }

	  		
	  public: const float& operator[](int i) const {return d[i];}
	  
	  public: float& operator[] (int i) {return d[i];}

#if 0
	  public: Vector3 operator* (const Vector3& v) const {
	    return Vector3 (d[0] * v.x + d[3] * v.y + d[6] * v.z,
	                    d[1] * v.x + d[4] * v.y + d[7] * v.z,
	                    d[2] * v.x + d[5] * v.y + d[8] * v.z);
    }
#endif
	  
	  public: Matrix3 operator* (const Matrix3& m) const;
  	
	  public: Matrix3& operator*= (const Matrix3& m) {
	    *this = *this * m;
      return *this;
    }	  

    public: Vector2 operator* (const Vector2& v) const {
  	  return Vector2(
        d[0]*v.x + d[3]*v.y + d[6],
	      d[1]*v.x + d[4]*v.y + d[7]
      );
    }

    public: Vector2 InverseTransform (const Vector2& point) const;

    private: void rotate (float i_x, float i_y, float i_z, float i_sin, float i_cos, float i_inv_cos);

    public: Matrix3& invert (void);
  };

  
} // namespace noz


#endif // __noz_Matrix3_h__

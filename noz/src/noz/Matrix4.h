///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Matrix4_h__
#define __noz_Matrix4_h__

#include "Vector2.h"

namespace noz {

  struct  Matrix4 {
    public: noz_float d[16];

    public: Matrix4 (void);

	  public: Matrix4 ( 
	    float i_00, float i_01, float i_02, float i_03, 
	    float i_10, float i_11, float i_12, float i_13, 
	    float i_20, float i_21, float i_22, float i_23,
	    float i_30, float i_31, float i_32, float i_33
	  );
	  
    public: Matrix4 (const Matrix4& i_mat);

	  public: Matrix4& identity (void) {
      d[1] = d[2] = d[3] = d[4] = d[6] = d[7] = d[8] = d[9] = d[11] = d[12] = d[13] = d[14] = 0.0f;
      d[0] = d[5] = d[10] = d[15] = 1.0f;
      return *this;
    }

    public: Matrix4& ortho (float l, float r, float b, float t, float n, float f);

	  public: Matrix4 transposed (void) const {
	    return Matrix4(	
	      d[0], d[4], d[8],  d[12],
		    d[1], d[5], d[9],  d[13],
		    d[2], d[6], d[10], d[14],
		    d[3], d[7], d[11], d[15]
	    );
    }

	  public: Matrix4& transpose (void) {
      *this = transposed();
      return *this;
    }

    public: Matrix4& invert (void);

    //public: Matrix4& rotate (const Vector3& i_around, float i_radians);    
    public: inline Matrix4& rotate (float i_x, float i_y, float i_z, float i_radians) {
      float s = Math::Sin (i_radians);
      float c = Math::Cos (i_radians);
      float t = 1.0f - c;
      rotate ( i_x, i_y, i_z, s, c, t );
      return *this;
    }
        

    //public: void scale (const Vector3& i_scale);    
    public: Matrix4& scale (float i_scale) {return scale (i_scale, i_scale, i_scale);}
    public: inline Matrix4& scale (float i_x, float i_y, float i_z) {
      Matrix4 scale ( i_x, 0.0f, 0.0f, 0.0f,
                      0.0f, i_y, 0.0f, 0.0f,
                      0.0f, 0.0f, i_z, 0.0f,
                      0.0f, 0.0f, 0.0f, 1.0f );
      *this = *this * scale;
      return *this;
    }



    //public: Matrix4& translate (const Vector3& v) {return translate(v.x,v.y,v.z);}
    public: inline Matrix4& translate (float x, float y, float z) {
      Matrix4 trans ( 1.0f,   0.0f,  0.0f,  0.0f, 
                      0.0f,   1.0f,  0.0f,  0.0f, 
                      0.0f,   0.0f,  1.0f,  0.0f, 
                      x, y, z, 1.0f );      
      *this = *this * trans;  
      return *this;
    }
    
       		
	  public: const float& operator[] (int i_index) const {return d[i_index];}
	  public: float& operator[] (int i_index) {return d[i_index];}

#if 0
	  public: Vector4 operator* (const Vector4& i_vec) const;
	    return Vector4( d[0] * i_vec[0] + d[4] * i_vec[1] + d[8]  * i_vec[2] + d[12] * i_vec[3],
	                    d[1] * i_vec[0] + d[5] * i_vec[1] + d[9]  * i_vec[2] + d[13] * i_vec[3],
	                    d[2] * i_vec[0] + d[6] * i_vec[1] + d[10] * i_vec[2] + d[14] * i_vec[3],
	                    d[3] * i_vec[0] + d[7] * i_vec[1] + d[11] * i_vec[2] + d[15] * i_vec[3]);
    }

	  public: Vector3 operator*	(const Vector3f& i_vec) const {
	    return Vector3f( d[0] * v.x + d[4] * v.y + d[8] * v.z + d[12],
	                     d[1] * v.x + d[5] * v.y + d[9] * v.z + d[13],
	                     d[2] * v.x + d[6] * v.y + d[10] * v.z + d[14]);
    }

#endif

	  public: Vector2 operator* (const Vector2& v) const {
  	  return Vector2(
        d[0]*v.x + d[4]*v.y + d[12],
	      d[1]*v.x + d[5]*v.y + d[13]
      );
    }

	  public: Matrix4 operator* (const Matrix4& i_mat) const;	
	  public: Matrix4& operator*= (const Matrix4& i_mat);

  
    private: inline void rotate (float x, float y, float z, float s, float c, float t) {
      Matrix4 rotate ( t * x * x + c,      t * x * y + s * z,    t * x * z - s * y,  0.0f,
                       t * x * y - s * z,  t * y * y + c,        t * y * z + s * x,  0.0f,
                       t * x * z + s * y,  t * y * z - s * x,    t * z * z + c,      0.0f,
                       0.0f,               0.0f,                 0.0f,               1.0f );      
      *this = *this * rotate;
    }
  };

} // namespace noz


#endif // __noz_Matrix4_h__

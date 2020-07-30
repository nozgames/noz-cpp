///////////////////////////////////////////////////////////////////////////////
// noZ Core
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;

Matrix4::Matrix4 (void) {
}

Matrix4::Matrix4 ( 
  float _00, float _01, float _02, float _03, 
	float _10, float _11, float _12, float _13, 
  float _20, float _21, float _22, float _23,
  float _30, float _31, float _32, float _33
  ) {
  d[0] = _00; d[1] = _01; d[2] = _02; d[3] = _03;
  d[4] = _10; d[5] = _11; d[6] = _12; d[7] = _13;
  d[8] = _20; d[9] = _21; d[10] = _22; d[11] = _23;
  d[12] = _30; d[13] = _31; d[14] = _32; d[15] = _33;
}


Matrix4::Matrix4 (const Matrix4& i_mat) {
  memcpy (d, i_mat.d, sizeof(d));
}  


Matrix4 Matrix4::operator* (const Matrix4& i_mat) const {
	int             i;
	int             j;
	const float*  m1Ptr;
	const float*  m2Ptr;
	float*        dstPtr;
	Matrix4      dst;

	m1Ptr = reinterpret_cast<const float*>(this);
	m2Ptr = reinterpret_cast<const float*>(&i_mat);
	dstPtr = reinterpret_cast<float*>(&dst);

	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			*dstPtr = 
			      m1Ptr[0] * m2Ptr[ 0 * 4 + j ]
					+ m1Ptr[1] * m2Ptr[ 1 * 4 + j ]
					+ m1Ptr[2] * m2Ptr[ 2 * 4 + j ]
					+ m1Ptr[3] * m2Ptr[ 3 * 4 + j ];
			dstPtr++;
		}
		m1Ptr += 4;
	}

	return dst;
}

Matrix4& Matrix4::operator*= (const Matrix4& i_mat) {
  int i;
  int j;

	for ( i = 0; i < 4; i++ ) {
	  float dest[4];
		for ( j = 0; j < 4; j++ ) {
		  dest[j] = 
  		  d[i*4+0] * i_mat.d[0+j] +
  		  d[i*4+1] * i_mat.d[4+j] +
  		  d[i*4+2] * i_mat.d[8+j] +
  		  d[i*4+3] * i_mat.d[12+j];
  	}
  	
  	d[i*4+0] = dest[0];
  	d[i*4+1] = dest[1];
  	d[i*4+2] = dest[2];
  	d[i*4+3] = dest[3];
	}
	
	return *this;
}


Matrix4& Matrix4::invert(void) {
  // our matrices are column major and can be quite confusing to access 
  // when stored in the typical, one-dimensional array often used by the API.
  // Here are some shorthand conversion macros, which convert a row/column 
  // combination into an array index.
    
  #define MAT(matIn,r,c) d[c*4+r]

  #define m11 MAT(matIn,0,0)
  #define m12 MAT(matIn,0,1)
  #define m13 MAT(matIn,0,2)
  #define m14 MAT(matIn,0,3)
  #define m21 MAT(matIn,1,0)
  #define m22 MAT(matIn,1,1)
  #define m23 MAT(matIn,1,2)
  #define m24 MAT(matIn,1,3)
  #define m31 MAT(matIn,2,0)
  #define m32 MAT(matIn,2,1)
  #define m33 MAT(matIn,2,2)
  #define m34 MAT(matIn,2,3)
  #define m41 MAT(matIn,3,0)
  #define m42 MAT(matIn,3,1)
  #define m43 MAT(matIn,3,2)
  #define m44 MAT(matIn,3,3)

  // Inverse = adjoint / det. (See linear algebra texts.)

  // pre-compute 2x2 dets for last two rows when computing
  // cofactors of first two rows.
  float d12 = (m31 * m42 - m41 * m32);
  float d13 = (m31 * m43 - m41 * m33);
  float d23 = (m32 * m43 - m42 * m33);
  float d24 = (m32 * m44 - m42 * m34);
  float d34 = (m33 * m44 - m43 * m34);
  float d41 = (m34 * m41 - m44 * m31);

  float tmp[16];
    
  tmp[0] =  (m22 * d34 - m23 * d24 + m24 * d23);
  tmp[1] = -(m21 * d34 + m23 * d41 + m24 * d13);
  tmp[2] =  (m21 * d24 + m22 * d41 + m24 * d12);
  tmp[3] = -(m21 * d23 - m22 * d13 + m23 * d12);

  // Compute determinant as early as possible using these cofactors.
  float det = m11 * tmp[0] + m12 * tmp[1] + m13 * tmp[2] + m14 * tmp[3];

  // Run singularity test.
  if( det == 0.0 ) {
    identity ( );
  } else {
    float invDet = 1.0f / det;
       
    // Compute rest of inverse.
    tmp[0] *= invDet;
    tmp[1] *= invDet;
    tmp[2] *= invDet;
    tmp[3] *= invDet;

    tmp[4] = -(m12 * d34 - m13 * d24 + m14 * d23) * invDet;
    tmp[5] =  (m11 * d34 + m13 * d41 + m14 * d13) * invDet;
    tmp[6] = -(m11 * d24 + m12 * d41 + m14 * d12) * invDet;
    tmp[7] =  (m11 * d23 - m12 * d13 + m13 * d12) * invDet;

    // Pre-compute 2x2 dets for first two rows when computing cofactors 
    // of last two rows.
    d12 = m11 * m22 - m21 * m12;
    d13 = m11 * m23 - m21 * m13;
    d23 = m12 * m23 - m22 * m13;
    d24 = m12 * m24 - m22 * m14;
    d34 = m13 * m24 - m23 * m14;
    d41 = m14 * m21 - m24 * m11;

    tmp[8]  =  (m42 * d34 - m43 * d24 + m44 * d23) * invDet;
    tmp[9]  = -(m41 * d34 + m43 * d41 + m44 * d13) * invDet;
    tmp[10] =  (m41 * d24 + m42 * d41 + m44 * d12) * invDet;
    tmp[11] = -(m41 * d23 - m42 * d13 + m43 * d12) * invDet;
    tmp[12] = -(m32 * d34 - m33 * d24 + m34 * d23) * invDet;
    tmp[13] =  (m31 * d34 + m33 * d41 + m34 * d13) * invDet;
    tmp[14] = -(m31 * d24 + m32 * d41 + m34 * d12) * invDet;
    tmp[15] =  (m31 * d23 - m32 * d13 + m33 * d12) * invDet;

    memcpy ( d, tmp, sizeof(float) * 16 );
  }
  
  #undef m11
  #undef m12
  #undef m13
  #undef m14
  #undef m21
  #undef m22
  #undef m23
  #undef m24
  #undef m31
  #undef m32
  #undef m33
  #undef m34
  #undef m41
  #undef m42
  #undef m43
  #undef m44
  #undef MAT    
  
  return *this;
}



Matrix4& Matrix4::ortho (float l, float r, float b, float t, float n, float f) {
  d[0] = 2.0f / (r - l);
  d[4] = 0.0f;
  d[8] = 0.0f;
  d[12] = -((r + l) / (r - l));
  
  d[1] = 0.0f;
  d[5] = 2.0f / (t - b);
  d[9] = 0.0f;
  d[13] = -((t + b) / (t - b));
  
  d[2] = 0.0f;
  d[6] = 0.0f;
  d[10] = -2.0f / (f - n);
  d[14] = -((f + n) / (f - n));
  
  d[3] = 0.0f;
  d[7] = 0.0f;  
  d[11] = 0.0f;
  d[15] = 1.0f;

  return *this;
}
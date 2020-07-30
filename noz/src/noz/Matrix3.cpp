///////////////////////////////////////////////////////////////////////////////
// noZ Core
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

using namespace noz;

Matrix3 Matrix3::Identity (1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

Matrix3::Matrix3 (const Matrix3& m) {
	memcpy (d, m.d, sizeof(d));
}

void Matrix3::rotate (float x, float y, float z, float s, float c, float t) {
  Matrix3 rotate ( 
    t * x * x + c,      t * x * y + s * z,    t * x * z - s * y,
    t * x * y - s * z,  t * y * y + c,        t * y * z + s * x,
    t * x * z + s * y,  t * y * z - s * x,    t * z * z + c
  );      
  
  *this = *this * rotate;
}

Matrix3 Matrix3::operator* ( const Matrix3& m ) const {
  int i;
  int j;
  const float* m1Ptr;
  const float* m2Ptr;
  float* dstPtr;
  Matrix3 dst;

  m1Ptr = reinterpret_cast<const float*>(this->d);
  m2Ptr = reinterpret_cast<const float*>(m.d);
  dstPtr = reinterpret_cast<float*>(dst.d);

  for ( i = 0; i < 3; i++ ) {
	  for ( j = 0; j < 3; j++ ) {
		  *dstPtr = m1Ptr[0] * m2Ptr[ 0 * 3 + j ]
				  + m1Ptr[1] * m2Ptr[ 1 * 3 + j ]
				  + m1Ptr[2] * m2Ptr[ 2 * 3 + j ];
		  dstPtr++;
	  }
	  m1Ptr += 3;
  }
  return dst;
}


Matrix3& Matrix3::invert(void) {
#if 0
  noz_float det=1.0f / (d[0]*(d[4]*d[8]-d[7]*d[5])-d[1]*(d[3]*d[8]-d[5]*d[6])+d[2]*(d[3]*d[7]-d[4]*d[6]));
	Matrix3 out;
  out.d[0]=(d[4]*d[8]-d[7]*d[5])* det;
	out.d[1]=-(d[3]*d[8]-d[5]*d[6])* det;
	out.d[2]=(d[3]*d[7]-d[6]*d[4])* det;
	out.d[3]=-(d[1]*d[8]-d[2]*d[7])* det;
	out.d[4]=(d[0]*d[8]-d[2]*d[6])* det;
	out.d[5]=-(d[0]*d[7]-d[6]*d[1])* det;
	out.d[6]=(d[1]*d[5]-d[2]*d[4])* det;
	out.d[7]=-(d[0]*d[5]-d[3]*d[2])* det;
	out.d[8]=(d[0]*d[4]-d[3]*d[1])* det;
#else
  noz_float det=1.0f / 
    ( d[0] * (d[8]*d[4]-d[7]*d[5]) - 
      d[3] * (d[8]*d[1]-d[7]*d[2]) + 
      d[6] * (d[5]*d[1]-d[4]*d[2]));

	Matrix3 out;
  out.d[0]= (d[8]*d[4]-d[7]*d[5])* det;
	out.d[1]=-(d[8]*d[1]-d[7]*d[2])* det;
	out.d[2]= (d[5]*d[1]-d[4]*d[2])* det;
	out.d[3]=-(d[8]*d[3]-d[6]*d[5])* det;
	out.d[4]= (d[8]*d[0]-d[6]*d[2])* det;
	out.d[5]=-(d[5]*d[0]-d[3]*d[2])* det;
	out.d[6]= (d[7]*d[3]-d[6]*d[4])* det;
	out.d[7]=-(d[7]*d[0]-d[6]*d[1])* det;
	out.d[8]= (d[4]*d[0]-d[3]*d[1])* det;
#endif
  *this = out;
  return *this;
}

Vector2 Matrix3::InverseTransform(const Vector2& point) const {
  noz_float det=1.0f / (d[0]*(d[4]*d[8]-d[7]*d[5])-d[1]*(d[3]*d[8]-d[5]*d[6])+d[2]*(d[3]*d[7]-d[4]*d[6]));
  noz_float inv[6] = {
     (d[4]*d[8]-d[7]*d[5]) * det,
	  -(d[3]*d[8]-d[5]*d[6]) * det,
	   (d[3]*d[7]-d[6]*d[4]) * det,
	  -(d[1]*d[8]-d[2]*d[7]) * det,
	   (d[0]*d[8]-d[2]*d[6]) * det,
	  -(d[0]*d[7]-d[6]*d[1]) * det
  };
  return Vector2(
    inv[0]*point.x + inv[1]*point.y + inv[2],
	  inv[3]*point.x + inv[4]*point.y + inv[5]
  );  
}

/*
Rect Matrix3::transform(const Rect& rect) const {
  Rect result(*this * Point(rect.left,rect.top));
  result.add(*this * Point(rect.left,rect.bottom));
  result.add(*this * Point(rect.right,rect.top));
  result.add(*this * Point(rect.right,rect.bottom));
  return result;
}
*/

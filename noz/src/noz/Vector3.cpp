///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <math.h>

using namespace noz;

Vector3 Vector3::Zero ( 0.0f, 0.0f, 0.0f );

Vector3::Vector3( const Vector2& v) {
  x = v.x; y = v.y; z = 0.0f;
}

float Vector3::normalize ( void ) {
  // Calculate the length of the vector		
  float l = sqrt ( (x*x) + (y*y) + (z*z) );

  // Prevent divide by zero crashes
  if ( l == 0.0f ) {
    return 0.0f;
  }
  
  float li = 1.0f / l;
  x *= li;
  y *= li;
  z *= li;
  return l;
}

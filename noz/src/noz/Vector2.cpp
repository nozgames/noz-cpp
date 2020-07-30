///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/StringReader.h>
#include <noz/IO/BinaryWriter.h>
#include <noz/IO/BinaryReader.h>
#include <noz/Text/StringLexer.h>

using namespace noz;

Vector2 Vector2::Empty;
Vector2 Vector2::Zero;
Vector2 Vector2::One(1.0f,1.0f);
Vector2 Vector2::OneZero(1.0f,0.0f);
Vector2 Vector2::ZeroOne(0.0f,1.0f);
Vector2 Vector2::NaN(Float::NaN, Float::NaN);

Vector2::Vector2 (const Vector3& v) {
  x = v.x; y = v.y;
}

Vector2 Vector2::Normalized (void) const {
  // Calculate the length of the vector		
  float l = Math::Sqrt ( (x*x) + (y*y) );

  // Prevent divide by zero crashes
  if ( l == 0.0f ) {
    return Vector2 ( 0.0f, 0.0f );
  }
  
  l = 1.0f / l;
  return Vector2 ( x * l, y * l );
}


float Vector2::normalize ( void ) {
  // Calculate the length of the vector		
  float l = Math::Sqrt ( (x*x) + (y*y) );

  // Prevent divide by zero crashes
  if ( l == 0.0f ) {
    return 0.0f;
  }
  
  float li = 1.0f / l;
  x *= li;
  y *= li;
  return l;
}

Vector2 Vector2::Parse(const char* s) {
  Vector2 result;
  StringReader reader(s);
  StringLexer lexer(&reader,",",StringLexer::FlagLiteralNumber);
  
  result.x = Float::Parse(lexer.Consume());

  if(!lexer.Consume(StringLexer::TokenType::Separator)) return result;
  
  result.y = Float::Parse(lexer.Consume());
  
  return result;
}

bool Vector2Object::Equals (Object* v) const {
  if(v==nullptr) return false;
  if(v==this) return true;

  switch(v->GetTypeCode()) {
    case TypeCode::Vector2: return ((Vector2Object*)v)->GetValue() == value_;
    default:
      break;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/StringReader.h>
#include <noz/Text/StringLexer.h>

using namespace noz;

Rect Rect::Empty;

Rect::Rect(void) {
  x = y = w = h = 0;
}

Rect::Rect(noz_float _x, noz_float _y, noz_float _w, noz_float _h) {
  x = _x; 
  y = _y;
  w = _w;
  h = _h;
}

Rect Rect::Union (const Rect& r) const {
  noz_float _l = Math::Min(r.x,x);
  noz_float _r = Math::Max(r.x+r.w,x+w);
  noz_float _t = Math::Min(r.y,y);
  noz_float _b = Math::Max(r.y+r.h,y+h);
  return Rect(_l,_t,_r-_l,_b-_t);
}

Rect Rect::Union (const Vector2& p) const {
  noz_float _l = Math::Min(p.x,x);
  noz_float _r = Math::Max(p.x,x+w);
  noz_float _t = Math::Min(p.y,y);
  noz_float _b = Math::Max(p.y,y+h);
  return Rect(_l,_t,_r-_l,_b-_t);
}

Rect Rect::Intersection(const Rect& r) const {
/*
  if(!intersects(r)) {
    return Rect(0,0,0,0);
  }
  return Rect (
    Math::Max(r.left,left),
    Math::Max(r.top,top),
    Math::Min(r.right,right),
    Math::Min(r.bottom,bottom)
  );
 */
  return Rect();
}

Rect Rect::Parse (const char* s) {
  Rect result;
  StringReader reader(s);
  StringLexer lexer(&reader,",",StringLexer::FlagLiteralNumber);
  
  if(!lexer.IsLiteralNumber()) return result;
  result.x = Float::Parse(lexer.Consume());
  if(!lexer.Consume(StringLexer::TokenType::Separator)) return result;
  
  if(!lexer.IsLiteralNumber()) return result;
  result.y = Float::Parse(lexer.Consume());
  if(!lexer.Consume(StringLexer::TokenType::Separator)) return result;

  if(!lexer.IsLiteralNumber()) return result;
  result.w = Float::Parse(lexer.Consume());
  if(!lexer.Consume(StringLexer::TokenType::Separator)) return result;
  
  if(!lexer.IsLiteralNumber()) return result;
  result.h = Float::Parse(lexer.Consume());
  return result;
}

bool Rect::Contains(const Vector2& pt) const {
  return pt.x>=x && pt.y>=y && pt.x <= (x+w) && pt.y <= (y+h);
}

bool Rect::Intersects (const Rect& r) const {
  if(x > r.x + r.w) return false;
  if(r.x > x + w) return false;
  if(y > r.y + r.h) return false;
  if(r.y > y + h) return false;
  return true;
}

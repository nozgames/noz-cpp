///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Rect_h__
#define __noz_Rect_h__

#include "Vector2.h"

namespace noz {

  class Rect {
    public: static Rect Empty;

    public: noz_float x;
    public: noz_float y;
    public: noz_float w;
    public: noz_float h;

    public: Rect(void);
    public: Rect(noz_float _x, noz_float _y, noz_float _w, noz_float _h);
    public: Rect(const Vector2& v) {*this = v;}

    public: bool empty(void) const {return !w && !h;}
    public: bool IsEmpty(void) const {return !w && !h;}

    public: Vector2 GetMin(void) const {return Vector2(x,y);}
    public: Vector2 GetMax(void) const {return Vector2(x+w,y+h);}
    public: Vector2 GetSize(void) const {return Vector2(w,h);}

    public: Vector2 GetCenter (void) const {return Vector2(x+w*0.5f,y+h*0.5f);}

    public: Vector2 GetTopLeft (void) const {return Vector2(x,y);}
    public: Vector2 GetBottomLeft (void) const {return Vector2(x,y+h);}
    public: Vector2 GetBottomRight (void) const {return Vector2(x+w,y+h);}
    public: Vector2 GetTopRight (void) const {return Vector2(x+w,y);}

    public: noz_float& operator[] (noz_int32 index) {return *((&x)+index);}
    public: const noz_float& operator[] (noz_int32 index) const{return *((&x)+index);}

    public: Rect& operator= (const Vector2& v) {x=v.x;y=v.y;w=0;h=0;return *this;}

    public: bool operator== (const Rect& r) const {return r.x==x&&r.y==y&&r.w==w&&r.h==h;}
    public: bool operator!= (const Rect& r) const {return !(*this==r);}

    public: Rect Intersection(const Rect& r) const;

    public: Rect Union (const Rect& r) const;
    public: Rect Union (const Vector2& p) const;

    public: bool Contains(const Vector2& pt) const;
    public: bool Intersects (const Rect& r) const;

    public: static Rect Parse (const char* s);
  };


} // namespace noz


#endif //__noz_Rect_h__



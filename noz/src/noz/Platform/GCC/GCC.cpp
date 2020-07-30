///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Math.h>

using namespace noz;

noz_int32 Math::CountLeadingZeroBits (noz_uint32 x) {
  if(x==0) return 32;
  return __builtin_clz(x);
}

noz_int32 Math::CountLeadingZeroBits (noz_uint64 x) {
  noz_int32 r = CountLeadingZeroBits((noz_uint32)(x>>32));
  if(r==32) { 
    return 32 + CountLeadingZeroBits((noz_uint32)(x&0xFFFFFFFF));
  }
  return r;
}

noz_int32 Math::CountTrailingZeroBits (noz_uint32 x) {
  if(x==0) return 32;
  return __builtin_ctz(x);
}

noz_int32 Math::CountTrailingZeroBits (noz_uint64 x) {  
  noz_int32 r = CountTrailingZeroBits((noz_uint32)(x&0xFFFFFFFF));
  if(r==32) {
    return 32 + CountTrailingZeroBits((noz_uint32)(x>>32));
  }
  return r;
}

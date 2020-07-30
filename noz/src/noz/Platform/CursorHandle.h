///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_CursorHandle_h__
#define __noz_Platform_CursorHandle_h__

namespace noz {
namespace Platform { 

  class CursorHandle : public Handle {
    public: static CursorHandle* CreateInstance(Cursor* cursor);
  };

} // namespace Platform
} // namespace noz


#endif // __noz_Platform_CursorHandle_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SystemCursor_h__
#define __noz_SystemCursor_h__

#include "Cursor.h"

namespace noz {

  NOZ_ENUM() enum class SystemCursorType {
    Arrow,
    Cross,
    Default,        
    SizeWE,
    SizeNS,
    SizeNESW,
    SizeNWSE,
    SizeAll,
    IBeam,
    Hand
  };

  class SystemCursor : public Cursor {
    NOZ_OBJECT(Managed)

    private: SystemCursorType cursor_type_;

    /// Default constructor
    public: SystemCursor (void);

    public: SystemCursor (SystemCursorType type);

    public: SystemCursorType GetCursorType(void) const {return cursor_type_;}
  };

} // namespace noz


#endif //__noz_SystemCursor_h__


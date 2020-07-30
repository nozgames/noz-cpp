///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Cursor_h__
#define __noz_Cursor_h__

namespace noz { namespace Platform { class CursorHandle; }}

namespace noz {

  class Cursor : public Asset {
    NOZ_OBJECT(Abstract,Managed)

    protected: Platform::CursorHandle* handle_;

    /// Default constructor
    protected: Cursor (void);

    public: ~Cursor (void);

    public: Platform::CursorHandle* GetHandle(void) const {return handle_;}
  };

} // namespace noz


#endif //__noz_Cursor_h__


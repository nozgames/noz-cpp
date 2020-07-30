///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Cursors_h__
#define __noz_Cursors_h__

#include "Cursor.h"

namespace noz {

  class Cursors {
    private: static Cursors* this_;

    public: static ObjectPtr<Cursor> Default;

    public: static ObjectPtr<Cursor> Cross;

    public: static ObjectPtr<Cursor> SizeNS;

    public: static ObjectPtr<Cursor> SizeWE;

    public: static ObjectPtr<Cursor> SizeNESW;

    public: static ObjectPtr<Cursor> SizeNWSE;

    public: static ObjectPtr<Cursor> SizeAll;

    public: static ObjectPtr<Cursor> IBeam;

    public: static ObjectPtr<Cursor> Hand;

    /// Hidden constructor
    private: Cursors (void);

    public: static void Initialize (void);

    public: static void Uninitialize (void);
  };

} // namespace noz


#endif //__noz_Cursors_h__


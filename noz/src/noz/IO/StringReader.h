///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_StringReader_h__
#define __noz_IO_StringReader_h__

#include "TextReader.h"

namespace noz {

  class StringReader : public TextReader {
    NOZ_OBJECT(NoAllocator)

    public: StringReader(const char* value);

    public: StringReader(String value);

    private: String value_;

    private: size_t position_;

    public: virtual noz_int32 Read(char* buffer, noz_int32 index, noz_int32 count) override;

    public: virtual noz_uint32 GetPosition (void) const override  {return position_;}
  };

} // namespace noz


#endif //__noz_IO_StringReader_h__


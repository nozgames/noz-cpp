///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_StreamWriter_h__
#define __noz_IO_StreamWriter_h__

#include "TextWriter.h"

namespace noz {

  class StreamWriter : public TextWriter {
    NOZ_OBJECT()

    private: Stream* stream_;

    /// Construct the writer using the given base stream.  Note the writer does
    /// not take ownership of the given base stream.
    public: StreamWriter(Stream* stream);

    public: ~StreamWriter(void);

    public: Stream* GetBaseStream(void) const {return stream_;}

    private: virtual void WriteInternal (const char* buffer, noz_int32 index, noz_int32 count) override;
  };

} // namespace noz


#endif //__noz_IO_StreamWriter_h__


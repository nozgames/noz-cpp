///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_StreamReader_h__
#define __noz_IO_StreamReader_h__

#include "TextReader.h"

namespace noz {

  class StreamReader : public TextReader {
    NOZ_OBJECT(NoAllocator)

    private: Stream* stream_;

    /// Construct the reader from the given base stream.  Note that the stream reader
    /// does not take ownership of the given stream.
    public: StreamReader(Stream* stream);

    public: ~StreamReader(void);

    public: Stream* GetBaseStream(void) const {return stream_;}

    public: virtual noz_int32 Read(void) override  {return TextReader::Read();}

    public: virtual noz_int32 Read(char* buffer, noz_int32 index, noz_int32 count) override {return stream_->Read(buffer,index,count);}

    public: virtual noz_uint32 GetPosition (void) const override  {return stream_->GetPosition();}

    public: virtual void SetPosition (noz_uint32 pos) override {peek_=0; stream_->Seek(pos,SeekOrigin::Begin);}

    private: void SkipBOM(void);
  };

} // namespace noz


#endif //__noz_IO_StreamReader_h__


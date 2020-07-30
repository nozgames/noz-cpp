///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_ZipArchive_h__
#define __noz_IO_ZipArchive_h__

namespace noz {

  class ZipArchive;

  class ZipArchiveEntry : public Object {
    private: ZipArchive* archive_;

    private: noz_uint64 compressed_length_;

    private: String full_name_;

    private: noz_uint64 lenght_;

    private: Name name_;
  };

  class ZipArchive : public Object {
    NOZ_OBJECT(NoAllocator)

    public: ZipArchive(Stream* stream);
  };

} // namespace noz


#endif //__noz_IO_ZipArchive_h__


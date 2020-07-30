///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_FileStream_h__
#define __noz_IO_FileStream_h__

namespace noz {

  enum class FileMode {
    Append,
    Create,
    CreateNew,
    Open,
    OpenOrCreate,
    Truncate,
  };

  enum class FileAccess {
    Read,
    ReadWrite,
    Write
  };

  class FileStreamHandle {
    public: static FileStreamHandle* CreateInstance(const String& path, FileMode mode, FileAccess _access);
    public: virtual ~FileStreamHandle(void) {}
    public: virtual noz_int32 Read(char* buffer, noz_int32 count) = 0;
    public: virtual noz_int32 Write(char* buffer, noz_int32 count) = 0;
    public: virtual noz_uint32 Seek(noz_int32 offset, SeekOrigin origin) = 0;
    public: virtual noz_uint32 GetPosition(void) const = 0;
    public: virtual noz_uint32 GetLength(void) const = 0;
  };

  class FileStream : public Stream {
    NOZ_OBJECT()

    public: static const noz_int32 DefaultBufferSize = 4096;

    private: String filename_;

    private: FileStreamHandle* handle_;
    
    public: FileStream (void);

    public: ~FileStream(void);

    public: bool Open(const String& path, FileMode mode, FileAccess access=FileAccess::ReadWrite);

    public: void Close (void);

    public: virtual noz_uint32 Seek(noz_int32 offset, SeekOrigin origin) override;

    public: virtual noz_uint32 GetPosition(void) const override;

    public: virtual noz_int32 Read(char* buffer, noz_int32 offset, noz_int32 count) override;

    public: virtual noz_int32 Write(char* buffer, noz_int32 offset, noz_int32 count) override;

    public: virtual noz_uint32 GetLength(void) const override;

    public: String GetFilename(void) const {return filename_;}
  };
  
} // namespace noz


#endif //__noz_IO_FileStream_h__


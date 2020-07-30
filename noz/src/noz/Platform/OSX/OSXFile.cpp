///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/Directory.h>
#include <noz/IO/Path.h>
#include <noz/IO/File.h>

#include <CoreFoundation/CoreFoundation.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

using namespace noz;

namespace noz { 
namespace Platform {
namespace OSX {

  class OSXFileStreamHandle : public FileStreamHandle {
    public: int handle_;

    public: OSXFileStreamHandle(const String& path, FileMode mode, FileAccess _access) {
      // Convert file flags into iphone file flags
      int flags = 0;
      if(mode == FileMode::Open) flags |= O_RDONLY; else flags |= (O_RDWR|O_CREAT);
      if(mode == FileMode::Truncate) flags |= O_TRUNC;

      // Open the file
      handle_ = ::open (path.ToCString(), flags, 0666);
    }

    public: ~OSXFileStreamHandle(void) {
      if(handle_ != -1) {
        ::close(handle_);
      } 
    }

    public: noz_int32 Read(char* buffer, noz_int32 count) {
      return (noz_int32)::read (handle_, buffer, count);
    }

    public: noz_int32 Write(char* buffer, noz_int32 count) {
      return (noz_int32)::write (handle_, buffer, count);
    }

    public: noz_uint32 Seek(noz_int32 offset, SeekOrigin origin) {
      switch(origin) {
        case SeekOrigin::Begin:   return (noz_int32)lseek (handle_, offset, SEEK_SET );
        case SeekOrigin::Current: return (noz_int32)lseek (handle_, offset, SEEK_CUR );
        case SeekOrigin::End:     return (noz_int32)lseek (handle_, offset, SEEK_END );
      }
      return 0;
    }

    public: noz_uint32 GetPosition(void) const {
      return (noz_uint32)lseek (handle_, 0, SEEK_CUR );
    }

    public: virtual noz_uint32 GetLength(void) const {
      auto pos = lseek(handle_,0,SEEK_SET);
      auto len = lseek(handle_,0,SEEK_END);
      lseek(handle_,pos,SEEK_SET);
      return (noz_uint32)len;
    }
  };

} // namespace OSX
} // namespace Platform
} // namespace noz



FileStreamHandle* FileStreamHandle::CreateInstance(const String& path, FileMode mode, FileAccess _access) {
  Platform::OSX::OSXFileStreamHandle* handle = new Platform::OSX::OSXFileStreamHandle(path,mode,_access);
  if(handle->handle_ == -1) {
    delete handle;
    return nullptr;
  }
  return handle;
}


bool File::Exists(const String& path) {
  struct stat sb;
  if(stat(path.ToCString(),&sb)==-1) {
    return false;
  }

  if(S_ISDIR(sb.st_mode)) return false;

  return true;
}

DateTime File::GetLastWriteTime (const String& path) {
  struct stat sb;
  if(stat(path.ToCString(),&sb)==-1) {
    return DateTime();
  }

  return DateTime(DateTime::DaysTo1970 * DateTime::TicksPerDay + sb.st_mtime * DateTime::TicksPerSecond);
}

bool File::Copy (const String& src, const String& dst, bool overwrite) {
  return false;
}


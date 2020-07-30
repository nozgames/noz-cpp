///////////////////////////////////////////////////////////////////////////////
// noZ Engine Framework
// Copyright (C) 2013-2014 Bryan Dube / Radius Software
// http://www.radius-software.com
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
#include <stdio.h>

using namespace noz;

namespace noz { 
namespace Platform {
namespace IOS {

  String GetIOSPath(const String& path) {
    CFBundleRef mainBundle = CFBundleGetMainBundle(); 
    CFURLRef url = CFBundleCopyBundleURL ( mainBundle );
      
    UInt8 buffer[2025];
    memset ( buffer, 0, sizeof(buffer) );
    CFURLGetFileSystemRepresentation ( url, true, buffer, 2025 );

    CFRelease ( url );
  
    String bpath ((const char*)buffer);
    
    return Path::Combine (bpath, path);
  }

  class IOSFileStreamHandle : public FileStreamHandle {
    public: int handle_;

    public: IOSFileStreamHandle(const String& path, FileMode mode, FileAccess _access) {

      // Convert file flags into iphone file flags
      int flags = 0;
      if(mode == FileMode::Open) flags |= O_RDONLY; else flags |= (O_RDWR|O_CREAT);
      if(mode == FileMode::Truncate) flags |= O_TRUNC;

      // Open the file
      handle_ = ::open (GetIOSPath(path).ToCString(), flags, 0777);
    }

    public: ~IOSFileStreamHandle(void) {
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

} // namespace IOS
} // namespace Platform
} // namespace noz



FileStreamHandle* FileStreamHandle::CreateInstance(const String& path, FileMode mode, FileAccess _access) {
  Platform::IOS::IOSFileStreamHandle* handle = new Platform::IOS::IOSFileStreamHandle(path,mode,_access);
  if(handle->handle_ == -1) {
    delete handle;
    return nullptr;
  }
  return handle;
}


bool File::Exists(const String& path) {
  struct stat sb;
  if(stat(Platform::IOS::GetIOSPath(path).ToCString(),&sb)==-1) {
    return false;
  }

  if(S_ISDIR(sb.st_mode)) return false;

  return true;
}

DateTime File::GetLastWriteTime (const String& path) {
  struct stat sb;
  if(stat(Platform::IOS::GetIOSPath(path).ToCString(),&sb)==-1) {
    return DateTime();
  }

  return DateTime(DateTime::DaysTo1970 * DateTime::TicksPerDay + sb.st_mtime * DateTime::TicksPerSecond);
}

bool File::Copy(const String& src, const String& dst, bool overwrite) {
  return false;
}

bool File::Delete(const String& path) {
  return 0==unlink(path.ToCString());  
}

bool File::Move (const String& src, const String& dst) {
  return 0==rename(src.ToCString(), dst.ToCString());  
}

std::vector<String> Font::GetFontFamilies (void) {
  return std::vector<String>();
}

std::vector<String> Font::GetFontStyles (const char* family) {
  return std::vector<String>();
}

bool Font::ReadFontData (const char* family, const char* style, noz_uint32 size, Stream* stream) {
  return false;
}

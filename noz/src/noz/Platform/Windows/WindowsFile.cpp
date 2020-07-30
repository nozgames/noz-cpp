///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/Directory.h>
#include <noz/IO/Path.h>
#include <noz/IO/File.h>

#include <Windows.h>
#undef CreateDirectory
#undef GetCurrentDirectory
#undef CopyFile

using namespace noz;

namespace noz { 
namespace Platform {
namespace Windows {

  class WindowsFileStreamHandle : public FileStreamHandle {
    public: HANDLE handle_;

    public: WindowsFileStreamHandle(const String& path, FileMode mode, FileAccess _access) {
      DWORD access = GENERIC_READ|GENERIC_WRITE;
      DWORD share = FILE_SHARE_READ|FILE_SHARE_READ;

      DWORD disposition = OPEN_EXISTING;
      if (mode==FileMode::Truncate || mode==FileMode::Create) {
        disposition = CREATE_ALWAYS;
      } else if (mode==FileMode::OpenOrCreate) {
        disposition = OPEN_ALWAYS;
      } else {
        disposition = OPEN_EXISTING;
      }

      // Create the windows file
	    handle_ = CreateFile ( 
        path.ToCString(),
	      access, 
	      share, 
	      NULL, 
	      disposition, 
        FILE_ATTRIBUTE_NORMAL, 
	      NULL 
	    );
    }

    public: ~WindowsFileStreamHandle(void) {
      if(handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(handle_);
      } 
    }

    public: noz_int32 Read(char* buffer, noz_int32 count) {
      DWORD read = 0;
      ReadFile (handle_, buffer, count, &read, NULL);
      return (noz_int32) read;
    }

    public: noz_int32 Write(char* buffer, noz_int32 count) {
      DWORD wrote = 0;
      WriteFile(handle_, buffer, count, &wrote, NULL);
      return (noz_int32) wrote;
    }

    public: noz_uint32 Seek(noz_int32 offset, SeekOrigin origin) {
      switch(origin) {
        case SeekOrigin::Begin:   return SetFilePointer (handle_, offset, NULL, FILE_BEGIN);
        case SeekOrigin::Current: return SetFilePointer (handle_, offset, NULL, FILE_CURRENT);
        case SeekOrigin::End:     return SetFilePointer (handle_, offset, NULL, FILE_END);
      }

      // TODO: exception
      return 0;
    }

    public: noz_uint32 GetPosition(void) const {
      return (noz_uint32)SetFilePointer (handle_, 0, NULL, FILE_CURRENT);
    }

    public: virtual noz_uint32 GetLength(void) const override {
      return (noz_uint32)GetFileSize(handle_,NULL);
    }
  };

} // namespace Windows
} // namespace Platform
} // namespace noz



FileStreamHandle* FileStreamHandle::CreateInstance(const String& path, FileMode mode, FileAccess _access) {
  Platform::Windows::WindowsFileStreamHandle* handle = new Platform::Windows::WindowsFileStreamHandle(path,mode,_access);
  if(handle->handle_ == INVALID_HANDLE_VALUE) {
    delete handle;
    return nullptr;
  }
  return handle;
}

static const noz_int32 DaysTo1601 = DateTime::DaysPer400Years * 4;

DateTime File::GetLastWriteTime (const String& path) {
  HANDLE handle = CreateFile ( 
    path.ToCString(),
	  GENERIC_READ, 
    FILE_SHARE_READ, 
	  NULL, 
    OPEN_EXISTING, 
    FILE_ATTRIBUTE_NORMAL, 
	  NULL 
  );

  FILETIME ft;
  if(!GetFileTime(handle, NULL, NULL, &ft)) {
    CloseHandle(handle);
    return DateTime();
  }

  CloseHandle(handle);

  return DateTime(DaysTo1601 * DateTime::TicksPerDay + (((noz_uint64)ft.dwHighDateTime) << 32) + (ft.dwLowDateTime));
}

bool File::Exists(const String& path) {
  DWORD ftyp = GetFileAttributesA(path.ToCString());
  if (ftyp == INVALID_FILE_ATTRIBUTES) return false;
  if (!(ftyp & FILE_ATTRIBUTE_DIRECTORY)) return true;
  return false;
}


bool File::Copy (const String& src, const String& dst, bool overwrite) {
  return !!CopyFileA(src.ToCString(), dst.ToCString(), !overwrite);
}


bool File::Delete (const String& path) {
  return !!DeleteFile(path.ToCString());
}

bool File::Move (const String& src, const String& dst) {
  return !!::MoveFile(src.ToCString(), dst.ToCString());
}

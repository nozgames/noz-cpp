///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>

#include <Windows.h>
#include <intrin.h>

using namespace noz;


noz_int32 Math::CountLeadingZeroBits (noz_uint32 x) {
  unsigned long r = 0;
  if(!_BitScanReverse(&r, x)) return 32;
  return 31-r;
}

noz_int32 Math::CountLeadingZeroBits (noz_uint64 x) {
  unsigned long r = 0;
  if(!_BitScanReverse(&r, (noz_uint32)(x>>32))) {
    if(!_BitScanReverse(&r, (noz_uint32)(x&0xFFFFFFFF))) {
      return 64;
    }
    return (31-r) + 32;
  }
  return 31-r;
}

noz_int32 Math::CountTrailingZeroBits (noz_uint32 x) {
  unsigned long r = 0;
  if(!_BitScanForward(&r, x)) return 32;
  return r;
}

noz_int32 Math::CountTrailingZeroBits (noz_uint64 x) {  
  unsigned long r = 0;
  if(!_BitScanForward(&r, (noz_uint32)(x&0xFFFFFFFF))) {
    if(!_BitScanForward(&r, (noz_uint32)(x>>32))) {
      return 64;
    }
    return r + 32;
  }
  return r;
}

int CALLBACK Test(const LOGFONT* lf, const TEXTMETRIC* tm, DWORD ft, LPARAM lparam) {
  if(ft==TRUETYPE_FONTTYPE && lf->lfCharSet==0) {
    std::vector<String>* result = (std::vector<String>*)lparam;
    if(lf->lfFaceName[0] != '@') {
      result->push_back((const char*)lf->lfFaceName);
    }
  }
  return 1;
}

int CALLBACK Test2(
  _In_ ENUMLOGFONT   *lpelf,
  _In_ NEWTEXTMETRIC *lpntm,
  _In_ DWORD         FontType,
  _In_ LPARAM        lparam
) {
  if(lpelf->elfLogFont.lfCharSet != ANSI_CHARSET) return 1;
  if(NULL == strstr((const char*)lpelf->elfLogFont.lfFaceName,"Arial")) return 1;
  std::vector<String>* result = (std::vector<String>*)lparam;
  result->push_back((const char*)lpelf->elfStyle);
  return 1;
}

int SortProc(const String& lhs, const String& rhs) {
  return lhs.CompareTo(rhs) < 0;
}

std::vector<String> Font::GetFontFamilies (void) {
  std::vector<String> result;
  HDC hdc = GetDC(NULL);
  EnumFontFamiliesEx(hdc,NULL,(FONTENUMPROC)Test,(LPARAM)&result,0);
  ReleaseDC(NULL,hdc);
  std::sort(result.begin(), result.end(), SortProc);
  return result;
}

std::vector<String> Font::GetFontStyles (const char* family) {
  std::vector<String> result;
  HDC hdc = GetDC(NULL);
  EnumFontFamilies(hdc,family,(FONTENUMPROC)Test2,(LPARAM)&result);
  ReleaseDC(NULL,hdc);
  return result;
}


int CALLBACK Test3(
  _In_ ENUMLOGFONT   *lpelf,
  _In_ NEWTEXTMETRIC *lpntm,
  _In_ DWORD         FontType,
  _In_ LPARAM        lparam
) {
  if(lpelf->elfLogFont.lfCharSet != ANSI_CHARSET) return 1;

  LOGFONT* out = (LOGFONT*)lparam;
  if(strcmp((char*)lpelf->elfStyle, out->lfFaceName)) return 1;

  *out = lpelf->elfLogFont;
  return 0;
}

bool Font::ReadFontData (const char* family, const char* style, noz_uint32 size, Stream* stream) {

  LOGFONT lf;
  lf.lfHeight = 0;
  strcpy_s(lf.lfFaceName,32,style);
  HDC hdc = GetDC(NULL);
  EnumFontFamilies(hdc,family,(FONTENUMPROC)Test3,(LPARAM)&lf);

  if(lf.lfHeight==0) return false;

  lf.lfHeight = size;
  HFONT font = CreateFontIndirect(&lf);

  HDC hdc2 = ::CreateCompatibleDC(NULL);
  SelectObject(hdc2,font);
  DWORD data_size = GetFontData(hdc2,0,0,NULL,0);
  std::vector<noz_byte> data;
  data.resize(data_size);
  GetFontData(hdc2,0,0,&data[0],data_size);

  DeleteDC(hdc2);
  ReleaseDC(NULL,hdc);

  stream->Write((char*)&data[0],0,data_size);

  return true;
}




#include <noz/Platform/ProcessHandle.h>
#undef GetCurrentDirectory

namespace noz {
namespace Platform {
namespace Windows {
  
  class WindowsProcessHandle : public Platform::ProcessHandle {
    public: PROCESS_INFORMATION pi_;

    public: ~WindowsProcessHandle(void) {
      CloseHandle(pi_.hThread);
      CloseHandle(pi_.hProcess);
    }

    public: virtual bool HasExited (void) const {
      if(INVALID_HANDLE_VALUE == pi_.hProcess) return false;
      return WAIT_TIMEOUT != WaitForSingleObject(pi_.hProcess,0);
    }

    public: virtual void WaitForExit (void) {
      if(INVALID_HANDLE_VALUE==pi_.hProcess) return;
      WaitForSingleObject(pi_.hProcess,INFINITE);
    }

    public: virtual bool WaitForExit (noz_uint32 milliseconds) {
      if(INVALID_HANDLE_VALUE==pi_.hProcess) return true;
      return WAIT_TIMEOUT!=WaitForSingleObject(pi_.hProcess,milliseconds);
    }
  };

}
}
}

Platform::ProcessHandle* Platform::ProcessHandle::CreateInstance(const char* path, const char* args) {
  String cmd = String::Format("%s %s", path, args);
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si,sizeof(si));
  ZeroMemory(&pi,sizeof(pi));
  if(!CreateProcess(NULL, (char*)cmd.ToCString(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, Environment::GetCurrentDirectory().ToCString(), &si, &pi)) return nullptr;

  Windows::WindowsProcessHandle* handle = new Windows::WindowsProcessHandle;
  handle->pi_ = pi;
  return handle;
}


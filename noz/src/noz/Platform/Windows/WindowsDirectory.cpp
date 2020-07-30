///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/Directory.h>
#include <noz/IO/Path.h>
#include <noz/Guid.h>

#include <Windows.h>
#undef CreateDirectory
#undef GetCurrentDirectory
#undef SetCurrentDirectory

#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "shlobj.h"

using namespace noz;

static void DirectoryGetFiles(const String& path, bool dirs, bool recursive, std::vector<String>& files) {
  WIN32_FIND_DATA ffd;
  HANDLE hfind; 

  hfind = FindFirstFile((Path::Combine(path,"*")).ToCString(), &ffd);
  if(hfind==INVALID_HANDLE_VALUE) {
    return;
  }

  do {
    if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if(dirs) {
        if(ffd.cFileName[0] != '.') {
          files.push_back(Path::Combine(path,ffd.cFileName));    
        }
      }
      if(recursive) {
        if(ffd.cFileName[0] != '.') {
          DirectoryGetFiles(Path::Combine(path,ffd.cFileName),dirs,true,files);
        }
      }
    } else if(!dirs) {
      files.push_back(Path::Combine(path,ffd.cFileName));    
    }

  } while(FindNextFile(hfind,&ffd));
}

std::vector<String> noz::Directory::GetFiles(String path,bool recursive) {
  std::vector<String> files;
  DirectoryGetFiles(path,false,recursive, files);
  return files;
}


std::vector<String> noz::Directory::GetDirectories(String path, bool recursive) {
  std::vector<String> files;
  DirectoryGetFiles(path,true,recursive,files);
  return files;
}

bool noz::Directory::Exists(const String& path) {
  DWORD ftyp = GetFileAttributesA(path.ToCString());
  if (ftyp == INVALID_FILE_ATTRIBUTES) return false;
  if (ftyp & FILE_ATTRIBUTE_DIRECTORY) return true;
  return false;
}

String noz::Environment::GetFolderPath(SpecialFolder folder) {
  switch(folder) {
    case SpecialFolder::Application: return GetCurrentDirectory();

    case SpecialFolder::ApplicationSupport: {
      TCHAR szPath[MAX_PATH] = "";
      if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szPath))) {
      }
      return Path::Combine(szPath,Environment::GetExecutableName());
    }

    case SpecialFolder::Document: {
      TCHAR szPath[MAX_PATH] = "";
      if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szPath))) {
      }
      return Path::Combine(szPath,Environment::GetExecutableName());
    }

    case SpecialFolder::Cache: {
      TCHAR szPath[MAX_PATH] = "";
      if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA , NULL, 0, szPath))) {
      }
      return Path::Combine(Path::Combine(szPath,Environment::GetExecutableName()),"Cache");
    }

    case SpecialFolder::Temp: {
      TCHAR szPath[MAX_PATH] = "";
      GetTempPath(MAX_PATH,szPath);
      return szPath;
    }
  }

  return String::Empty;
}

String Environment::GetCurrentDirectory(void) {
  char buffer[MAX_PATH];
  if(0==::GetCurrentDirectoryA(MAX_PATH,buffer)) {
    return String::Empty;
  }

  return String(buffer);
}

void Environment::SetCurrentDirectory (const String& dir) {
  SetCurrentDirectoryA(dir.ToCString());
}

bool Directory::CreateDirectoryInternal(const String& path) {
  // Get directory name from given path.
  String dir=Path::GetDirectoryName(path);
  if(!dir.IsEmpty()) {
    if(!Exists(dir)) {
      if(!CreateDirectoryInternal(dir)) return false;
    }
  }
  
  int result=CreateDirectoryA(path.ToCString(),NULL);
  if(!result && GetLastError() != ERROR_ALREADY_EXISTS) {
    return false;
  }
  
  return true;
}


const char Path::PathSeparator = '\\';

bool Path::IsPathSeparator (const char c) {
  return c=='\\' || c=='/';
}


Guid Guid::Generate (void) {
  UUID uuid;
  if(RPC_S_OK!=UuidCreate(&uuid)) return Guid();

  noz_uint64 h = (((noz_uint64)uuid.Data1) << 32) + (((noz_uint64)uuid.Data2)<<16) + ((noz_uint64)uuid.Data3);
  noz_uint64 l = 
    (((noz_uint64)uuid.Data4[0])<<(8*7)) + 
    (((noz_uint64)uuid.Data4[1])<<(8*6)) + 
    (((noz_uint64)uuid.Data4[2])<<(8*5)) + 
    (((noz_uint64)uuid.Data4[3])<<(8*4)) + 
    (((noz_uint64)uuid.Data4[4])<<(8*3)) + 
    (((noz_uint64)uuid.Data4[5])<<(8*2)) + 
    (((noz_uint64)uuid.Data4[6])<<(8*1)) + 
    (((noz_uint64)uuid.Data4[7])<<(8*0));

  return Guid(h,l);
}

bool Directory::Delete (const String& path) {
  SHFILEOPSTRUCT s;
  ZeroMemory(&s,sizeof(s));

  char ospath[MAX_PATH];
  memset(ospath,0,MAX_PATH);
  strcpy_s(ospath, MAX_PATH, Path::GetWindowsPath(path).ToCString());
  ospath[path.GetLength()+1] = 0;

  s.wFunc = FO_DELETE;
  s.pFrom = ospath;
  s.fFlags = FOF_NOCONFIRMATION | FOF_NO_UI | FOF_SILENT;
  
  return 0==SHFileOperation(&s);
}

bool Directory::Move (const String& from, const String& to) {
  SHFILEOPSTRUCT s;
  ZeroMemory(&s,sizeof(s));

  char ospath[MAX_PATH];
  strcpy_s(ospath, MAX_PATH, Path::GetWindowsPath(from).ToCString());
  ospath[from.GetLength()+1] = 0;

  char ospath_to[MAX_PATH];
  strcpy_s(ospath_to, MAX_PATH, Path::GetWindowsPath(to).ToCString());
  ospath_to[to.GetLength()+1] = 0;

  s.wFunc = FO_MOVE;
  s.pFrom = ospath;
  s.pTo = ospath_to;
  s.fFlags = FOF_NOCONFIRMATION | FOF_NO_UI | FOF_SILENT;
  
  return 0==SHFileOperation(&s);
}

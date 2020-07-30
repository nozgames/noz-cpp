///////////////////////////////////////////////////////////////////////////////
// noZ Engine Framework
// Copyright (C) 2013-2014 Bryan Dube / Radius Software
// http://www.radius-software.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/IO/Directory.h>
#include <noz/IO/Path.h>
#include <noz/Guid.h>

#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>

using namespace noz;

namespace noz { 
namespace Platform {
namespace IOS {

  String GetIOSPath(const String& path);
}
}
}


static void DirectoryGetFiles(const String& _path, bool recursive, std::vector<String>& files) {
  String path = _path;

  DIR *dp;
  struct dirent *dirp;

  if((dp = opendir(path.ToCString())) == NULL) {
    return files;
  }

  while ((dirp = readdir(dp)) != NULL) {
    if(dirp->d_type == DT_DIR) {
      if(!recursive) continue;

      if(dirp->d_name[0] == '.' && (!dirp->d_name[1] || (dirp->d_name[1]=='.' && !dirp->d_name[2]))) {
        continue;
      }
      DirectoryGetFiles(Path::Combine(path,String(dirp->d_name)),true,files);
      continue;
    }
    if(dirp->d_type != DT_REG) continue;

    files.push_back(Path::Combine(path,String(dirp->d_name)));
  }

  closedir(dp);
}


std::vector<String> noz::Directory::GetFiles(String path,bool recursive) {
  std::vector<String> files;
  DirectoryGetFiles(path,recursive,files);
  return files;
}


std::vector<String> noz::Directory::GetDirectories(String path, bool recursive) {
  std::vector<String> files;
  return files;
}

bool noz::Directory::Exists(const String& _path) {
  struct stat sb;
  String path = Platform::IOS::GetIOSPath(_path);
  if(stat(path.ToCString(),&sb)==-1) {
    return false;
  }

  return S_ISDIR(sb.st_mode);
}



String Environment::GetCurrentDirectory(void) {
#if 1
  return Platform::IOS::GetIOSPath("");
#else
  char buffer[1024];
  return getcwd(buffer,1024);
#endif
}

void Environment::SetCurrentDirectory (const String& dir) {
  //SetCurrentDirectoryA(dir.ToCString());
}

const char Path::PathSeparator = '/';

bool Path::IsPathSeparator (const char c) {
  return c=='\\' || c=='/';
}

bool Directory::Move (const String& src, const String& dst) {
  return 0==rename(src.ToCString(), dst.ToCString());  
}

bool Directory::Delete (const String& path) {
  return 0==unlink(path.ToCString());  
}

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

#include <CoreFoundation/CoreFoundation.h>

using namespace noz;

static void DirectoryGetFiles(const String& path, bool recursive, std::vector<String>& files) {
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

bool noz::Directory::Exists(const String& path) {
  struct stat sb;
  if(stat(path.ToCString(),&sb)==-1) {
    return false;
  }

  return S_ISDIR(sb.st_mode);
}

String noz::Environment::GetFolderPath(SpecialFolder folder) {
  return "";
}


String noz::Environment::GetCurrentDirectory(void) {
  char buffer[1024];
  return getcwd(buffer,1024);
}

bool Directory::CreateDirectoryInternal(const String& path) {
  // Get directory name from given path.
  String dir=Path::GetDirectoryName(path);
  if(!dir.IsEmpty()) {
    if(!Exists(dir)) {
      if(!CreateDirectoryInternal(dir)) return false;
    }
  }
  
  int result = mkdir(path.ToCString(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  if(!result && result != EEXIST) {
    return false;
  }
  
  return true;
}

const char Path::PathSeparator = '/';

bool Path::IsPathSeparator (const char c) {
  return c=='\\' || c=='/';
}

Guid Guid::Generate(void) {
  CFUUIDRef theUUID = CFUUIDCreate(NULL);
  CFStringRef string = CFUUIDCreateString(NULL, theUUID);
  Guid result = Guid::Parse(CFStringGetCStringPtr(string,kCFStringEncodingUTF8));
  CFRelease(theUUID);
  CFRelease(string);
  return result;
}


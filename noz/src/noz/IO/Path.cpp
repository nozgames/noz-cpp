///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include "Path.h"
#include "Directory.h"
#include "StringReader.h"
#include <noz/Text/StringLexer.h>

using namespace noz;

String Path::ChangeExtension(const String& path,const String& ext) {
  // Find the last dot and last slash
  noz_int32 slash = Math::Max(path.LastIndexOf('\\'),path.LastIndexOf('/'));
  noz_int32 dot = path.LastIndexOf('.');

  if(ext.IsEmpty()) {
    if(dot==-1 || slash > dot) {
      return path;
    }
    return path.Substring(0,dot);
  }

  StringBuilder sb;
  if(dot==-1 || slash > dot) {
    sb.Append(path);
    if(ext[0]!='.' && path[path.GetLength()-1] != '.') sb.Append('.');
    sb.Append(ext);
  } else {
    sb.Append(path.Substring(0,dot));
    if(ext[0]!='.') sb.Append('.');
    sb.Append(ext);
  }
  
  return sb.ToString();
}


String Path::GetExtension(String path) {
  // Find last slash or backslash
  noz_int32 slash = Math::Max(path.LastIndexOf('\\'),path.LastIndexOf('/'));

  // Find last dot
  noz_int32 dot = path.LastIndexOf('.');

  // If the last dot is before the last slash then there is no extension.
  if(dot <= slash) {
    return String::Empty;
  }

  // Return the extension including the dot..
  return path.Substring(dot);
}

String Path::GetDirectoryName(String path) {
  // Find last slash or backslash
  noz_int32 slash = Math::Max(path.LastIndexOf('\\'),path.LastIndexOf('/'));

  if(slash<1) {
    return String::Empty;
  }

  // Make sure the directory name is not just a drive letter
  if(path.ToCString()[slash-1] == ':') {
    return String::Empty;
  }

  return path.Substring(0,slash);
}

String Path::Combine (const String& path1, const String& path2) {
  // Handle cases where one of the two strings is empty
  if(path1.IsEmpty()) return path2;
  if(path2.IsEmpty()) return path1;

  // If the second path has a drive letter then it overrides the first
  if(path2[1] == ':') return path2;

  // Handle the case where the second path is rooted
  String path2_root = GetPathRoot(path2);
  if(!path2_root.IsEmpty()) {
    return Combine(GetPathRoot(path1),path2.Substring(path2_root.GetLength()));
  }

  char last = path1[path1.GetLength()-1];
  StringBuilder sb;
  sb.Append (path1);
  if(last != '/' && last != '\\' && path2[0]!='/' && path2[0]!='\\') sb.Append('/');
  sb.Append (path2);
  return sb.ToString();
}

String Path::GetFilename (const String& path) {
  // Find last slash or backslash
  noz_int32 slash = Math::Max(path.LastIndexOf('\\'),path.LastIndexOf('/'));
  if(slash==-1) {
    return path;
  }

  return path.Substring(slash+1);
}

String Path::GetFilenameWithoutExtension (const String& path) {
  String filename = GetFilename(path);
  if(filename.IsEmpty()) return filename;

  noz_int32 dot = filename.LastIndexOf('.');
  if(dot != -1) {
    return filename.Substring(0,dot);
  }

  return filename;
}

bool Path::IsPathRooted(const String& path) {
  if(path.IsEmpty()) return false;
  if(path[0] == '\\' || path[0] == '/') return true;
  if(path[1] == ':') return true;
  return false;
}


String Path::GetPathRoot (const String& path) {
  if(path.IsEmpty()) return String::Empty;
  if(path[0] == '/') return "/";
  if(path[0] == '\\') return "\\";
  if(path[1] == ':') return String::Format("%c%c%c", path[0], path[1], '/');
  return String::Empty;
}

String Path::GetFullPath (const String& path) {
  return Path::Combine(Environment::GetCurrentDirectory(), path);
}


String Path::Canonical (const String& path) {
  StringReader reader(path);
  StringLexer lexer(&reader, "\\/", 0, "");
  StringBuilder sb;
  std::vector<String> paths;

  if(lexer.Consume(StringLexer::TokenType::Separator)) {
    sb.Append('/');
  }

  while(!lexer.IsEnd()) {
    if(lexer.Consume(StringLexer::TokenType::Separator)) continue;
    String path = lexer.Consume();
    if(path[0] == '.') {
      if(path[1] == '.') {
        if(!paths.empty()) {
          paths.pop_back();
        } else {
          sb.Append("../");
        }
        continue;
      } else if (path[1]==0) {
        continue;
      }
    }
    paths.push_back(path);
  }

  for(noz_uint32 i=0;i<paths.size()-1;i++) {
    sb.Append(paths[i]);
    sb.Append('/');
  }

  sb.Append(paths[paths.size()-1]);

  return sb.ToString();
}

String Path::GetRelativePath (const String& path, const String& _target) {
  StringBuilder result;

  // Start with the target without any trailing directory separators.
  String target;
  if(IsPathSeparator(_target[_target.GetLength()])) {
    target = _target.Substring(0,_target.GetLength()-2);
  } else {
    target = _target;
  }

  while(!target.IsEmpty()) {
    // Is there a match for the target path in the beginning of the path?
    if(path.StartsWith(target,StringComparison::OrdinalIgnoreCase) && IsPathSeparator(path[target.GetLength()])) {
      // Add the relative portion of the path to the accumulated result.
      result.Append(path.Substring(target.GetLength()+1));
      return result.ToString();
    }

    // Add the the relative path to the result
    result.Append("..");
    result.Append(Path::PathSeparator);
    
    // Remove directory from target
    target = Path::GetDirectoryName(target);
  }

  return String::Empty;
}

String Path::GetUnixPath (const String& path) {
  StringBuilder sb;
  for(noz_int32 s=0; s<path.GetLength(); ) {
    noz_int32 e = path.IndexOf('\\', s);
    if(e!=-1) {
      sb.Append(path.Substring(s,e-s));
      sb.Append('/');
      s = e+1;
    } else {
      sb.Append(path.Substring(s));
      break;
    }
  }
  return sb.ToString();
}

String Path::GetWindowsPath (const String& path) {
  StringBuilder sb;
  for(noz_int32 s=0; s<path.GetLength(); ) {
    noz_int32 e = path.IndexOf('/', s);
    if(e!=-1) {
      sb.Append(path.Substring(s,e-s));
      sb.Append('\\');
      s = e+1;
    } else {
      sb.Append(path.Substring(s));
      break;
    }
  }
  return sb.ToString();
}



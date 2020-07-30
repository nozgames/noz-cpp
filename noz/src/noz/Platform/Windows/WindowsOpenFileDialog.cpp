///////////////////////////////////////////////////////////////////////////////
// noZ Engine Framework
// Copyright (C) 2013-2014 Bryan Dube / Radius Software
// http://www.radius-software.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch>

#if 0
#include <noz/UI/Controls/OpenFileDialog.h>
#include "WindowsWindow.h"

#include <Commdlg.h>

using namespace noz;
using namespace noz::Platform::Windows;
using namespace noz::UI::Controls;


String OpenFileDialogHandle::ShowDialog (const String& file, const String& filter, noz_int32 filter_index_) {
#if 0
  char filename[MAX_PATH]="";

  if(!file.IsEmpty()) {
    strcpy_s(filename,MAX_PATH,file.ToCString());
  }

  char* f;
  if(filter.GetLength()) {
    f = new char[filter.GetLength()+2];
    strcpy_s(f,filter.GetLength()+2,filter.ToCString());
    f[filter.GetLength()+1] = 0;
    for(char* p=f;*p;p++) {
      if(*p=='|') *p = 0;
    }
  } else {
    f = new char[20];
    memcpy(f,"All Files(*.*)\0*.*\0\0",20);
  }

  OPENFILENAME ofn={0};
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner = ((WindowsWindow*)Application::GetMainWindow()->GetHandle())->cached_->hwnd; 
  ofn.nMaxFile = sizeof(filename);
  ofn.lpstrFile = filename;
  ofn.lpstrFilter = f;
  ofn.nFilterIndex = 0;
  ofn.Flags = OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;

  int result = GetOpenFileName(&ofn);

  delete[] f;

  return result==0 ? "" : filename;
#else
  return "";
#endif
}

#endif

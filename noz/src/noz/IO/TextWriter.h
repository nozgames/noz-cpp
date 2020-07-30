///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_IO_TextWriter_h__
#define __noz_IO_TextWriter_h__

namespace noz {

  class TextWriter : public Object {
    NOZ_OBJECT()

    public: TextWriter(void); 

    public: void Write (const String& v) {WriteInternal(v.ToCString(), (noz_int32)0, v.GetLength());}

    public: void Write (const Name& v) {if(v.IsEmpty()) return; Write(v.ToString());;}

    public: void Write (const char* v) {WriteInternal(v,0,String::GetLength(v));}
    
    public: void Write (char v) {WriteInternal(&v, 0, 1);}

    public: void Write (char v, noz_int32 count) {for(;count>0;count--) WriteInternal(&v,0,1);}

    public: void WriteLine (const String& v) {WriteInternal(v.ToCString(), (noz_int32)0, v.GetLength());Write("\n");}

    public: void WriteLine (const Name& v) {if(v.IsEmpty()) return; WriteLine(v.ToString());}

    public: void WriteLine (const char* v) {WriteInternal(v,0,String::GetLength(v));Write("\n");}

    public: void WriteLine (char v) {WriteInternal(&v, 0, 1);Write("\n");}

    protected: virtual void WriteInternal (const char* buffer, noz_int32 index, noz_int32 count) = 0;
  };

} // namespace noz


#endif //__noz_IO_TextWriter_h__


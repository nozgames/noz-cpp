///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Console_h__
#define __noz_Console_h__

namespace noz {

  class ConsoleVariable {
    friend class Console;

    private: Name name_;

    public: virtual ~ConsoleVariable (void) {}

    public: virtual void SetFloat (noz_float f) = 0;
    public: virtual void SetString (const String& s) = 0;
    public: virtual void SetInt32 (noz_int32 i) = 0;
    public: virtual void SetBoolean (bool b) = 0;
    
    public: virtual noz_int32 GetInt32(void) const = 0;

    public: ConsoleVariable (const Name& name);
  };

  class Int32ConsoleVariable : public ConsoleVariable {
    private: noz_int32 value_;
    private: noz_int32 min_value_;
    private: noz_int32 max_value_;

    public: Int32ConsoleVariable(const Name& name, noz_int32 value, noz_int32 minv, noz_int32 maxv) : ConsoleVariable(name) {
      min_value_ = minv;
      max_value_ = maxv;
      SetInt32(value);
    }

    public: noz_int32 GetValue(void) const {return value_;}

    public: virtual void SetFloat (noz_float f) {SetInt32((noz_int32)f);}
    public: virtual void SetString (const String& s) {SetInt32(Int32::Parse(s));}
    public: virtual void SetInt32 (noz_int32 i) {value_ = Math::Clamp(i,min_value_,max_value_);}
    public: virtual void SetBoolean (bool b) {SetInt32((noz_int32)b);}

    public: virtual noz_int32 GetInt32(void) const {return value_;}
  };

  enum class ConsoleMessageType {
    Error,
    Warning,
    Info
  };

  typedef Event<ConsoleMessageType,Object*,const char*> ConsoleMessageEventHandler;

  class Console {
    friend class ConsoleVariable;

    public: static ConsoleMessageEventHandler ConsoleMessageWritten;

    private: static Console* this_;

    private: std::map<Name,ConsoleVariable*> vars_;

    public: ~Console (void);

    public: static void Initialize (void);

    public: static void Uninitialize (void);

    public: static Int32ConsoleVariable* RegisterInt32Variable (const Name& name, noz_int32 value, noz_int32 value_min, noz_int32 value_max);

    public: static ConsoleVariable* FindVariable (const Name& var);

    public: static noz_int32 GetVariableInt32 (const Name& var);
    public: static void SetVariableInt32 (const Name& var, noz_int32 v);

    public: static void WriteLine(const char* format, ...);
    public: static void WriteLine(Object* context, const char* format, ...);

    public: static void WriteError (const char* format, ...);
    public: static void WriteError (Object* context, const char* format, ...);

    public: static void WriteWarning (const char* format, ...);
    public: static void WriteWarning (Object* context, const char* format, ...);

    private: static void WriteLine(ConsoleMessageType type, Object* context, const char* msg);
  };

}

#endif // __noz_Console_h__

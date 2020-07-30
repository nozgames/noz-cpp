///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_h__
#define __noz_h__

#define _HAS_ITERATOR_DEBUGGING 0

#include <vector>
#include <list>
#include <map>
#include <set>
#include <cstdarg>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#if NOZ_DEBUG
#include <assert.h>
#define noz_assert(x) noz_assert_func(!!(x))
inline void noz_assert_func(bool value) {
  if(!value) {
    assert(false);
  }
}
//#define noz_assert assert
#else
#define noz_assert(x)
#endif

#define NOZ_STRINGIZE_INTERNAL(x) #x
#define NOZ_STRINGIZE(x) NOZ_STRINGIZE_INTERNAL(x)

#define NOZ_TODO_ENABLED 0

#if NOZ_TODO_ENABLED && defined(NOZ_WINDOWS)
#define NOZ_TODO(msg) __pragma(message(__FILE__ "(" NOZ_STRINGIZE(__LINE__) "): todo: " msg ))
#else
#define NOZ_TODO(msg)
#endif
#define NOZ_FIXME() NOZ_TODO("fixme")

typedef unsigned char       noz_byte;
typedef char                noz_int8;
typedef short               noz_int16;
typedef int                 noz_int32;
typedef long long           noz_int64;
typedef unsigned char       noz_uint8;
typedef unsigned short      noz_uint16;
typedef unsigned int        noz_uint32;
typedef unsigned long long  noz_uint64;
typedef float               noz_float;
typedef double              noz_double;

typedef wchar_t             noz_wchar;
typedef size_t              noz_size;

#if defined(NOZ_WINDOWS)
#if defined(_WIN64)
typedef long long           noz_int;
typedef unsigned long long  noz_uint;
#else
typedef int                 noz_int;
typedef unsigned int        noz_uint;
#endif
#else
typedef long long           noz_int;
typedef unsigned long long  noz_uint;
#endif

#define NOZ_BIT(i) (1<<(i))

namespace noz {
  class Type;
  class Property;

  void ApplicationBegin (void);

  void ApplicationEnd (void);

  struct Attributes {
    noz_uint32 value;
    Attributes(void) {}
    Attributes(noz_uint32 _value) {value=_value;}
    operator noz_uint32 (void) const {return value;}
  };
}

#if defined(NOZ_LEAN)
#define NOZ_OBJECT_BASE(...)
#define NOZ_OBJECT(...) \
  public: virtual Type* GetTypeInternal(void) const override {return nullptr;}
#define NOZ_INTERFACE(...) 
#define typeof(x) ((Type*)nullptr)
#else
#define NOZ_OBJECT_BASE(...) \
  public: static noz::Type* type__; \
  public: static void RegisterType(void);
#define NOZ_OBJECT(...) \
  public: static noz::Type* type__; \
  public: static void RegisterType(void); \
  private: virtual Type* GetTypeInternal(void) const override {return type__;}

#define NOZ_INTERFACE(...) NOZ_OBJECT(__VAR_ARGS__)

#define typeof(x)  x::type__

#endif

#define NOZ_TEMPLATE(...)
#define NOZ_PROPERTY(...) 
#define NOZ_CONTROL_PART(...)
#define NOZ_ENUM(...)
#define NOZ_METHOD(...)
#define NOZ_GLUE_TARGET(...)


#include "Text/String.h"
#include "Name.h"
#include "Reflection/Type.h"
#include "Object.h"
#include "Reflection/Property.h"
#include "Reflection/Method.h"
#include "Text/StringObject.h"
#include "Event.h"
#include "Environment.h"
#include "Prefs.h"
#include "DateTime.h"
#include "Enum.h"
#include "Guid.h"
#include "System/Thread.h"
#include "System/Mutex.h"

namespace noz {
  class Handle {
    public: virtual ~Handle(void) {}
  };


  NOZ_ENUM() enum class Orientation {
    Horizontal,
    Vertical
  };

  NOZ_ENUM() enum class Alignment {
    Min,
    Center,
    Max
  }; 

  struct Axis {
    static const noz_int32 X = 0;
    static const noz_int32 Y = 1;
    static const noz_int32 Horizontal = 0;
    static const noz_int32 Vertical = 1;
  };

}

#include "Collections/LinkedList.h"
#include "Collections/ObjectArray.h"
#include "Text/String.h"
#include "Time.h"
#include "Delegate.h"
#include "Math.h"
#include "SystemEvent.h"
#include "UI/DragDrop.h"
#include "Console.h"
#include "IO/IO.h"
#include "Environment.h"

#if !defined(NOZ_LEAN)
#include "Serialization/SerializedObject.h"
#include "NodePath.h"
#include "Assets/AssetManager.h"
#include "Factory.h"
#include "Render/RenderContext.h"
#include "Audio/AudioClip.h"
#include "UI/Application.h"
#include "Animation/Animation.h"
#include "Diagnostics/Debugger.h"
#include "UI/Style.h"
#include "Scene.h"
#include "Nodes/Prefab.h"
#include "Nodes/PrefabNode.h"
#include "Reflection/Properties/Properties.h"
#include "Animation/Animator.h"
#include "Nodes/Layout/Layout.h"
#include "Nodes/Render/TextNode.h"
#include "Nodes/Render/SpriteNode.h"
#include "Nodes/Render/ImageNode.h"
#include "Nodes/Render/RectangleNode.h"
#include "Nodes/UI/Control.h"
#include "Nodes/UI/ContentControl.h"
#include "Nodes/UI/Popup.h"

#endif

#if defined(NOZ_EDITOR)
#include "Editor/EditorApplication.h"
#include "Editor/Asset/AssetDatabase.h"
#endif


#endif // __noz_h__


///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenAL_h__
#define __noz_Platform_OpenAL_h__

#define AL_LIBTYPE_STATIC

#if defined(NOZ_WINDOWS)
#include <external/openal-soft-1.15.1/include/AL/al.h>
#include <external/openal-soft-1.15.1/include/AL/alc.h>

#elif defined(NOZ_IOS)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#include "OpenALManager.h"
#include "OpenALAudioClip.h"
#include "OpenALAudioChannel.h"

#endif // __noz_Platform_OpenAL_h__

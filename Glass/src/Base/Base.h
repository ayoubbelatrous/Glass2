#pragma once
#include <memory>


#ifdef GS_DEBUG
#if defined(GS_WINDOWS)
#define GS_DEBUGBREAK() __debugbreak()
#elif defined(GS_LINUX)
#include <signal.h>
#define GS_DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debugbreak yet!"
#endif
#else
#define GS_DEBUGBREAK()
#endif

#define GS_EXPAND_MACRO(x) x
#define GS_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#include "Base/Assert.h"
#include "Base/Log.h"
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

namespace Glass
{
	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Weak = std::weak_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Weak<T> CreateWeak(Args&& ... args)
	{
		return std::weak_ptr<T>(std::forward<Args>(args)...);
	}
}


#include "Base/Assert.h"
#include "Base/Log.h"